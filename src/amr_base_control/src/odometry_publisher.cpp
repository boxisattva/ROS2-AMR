/**
 * odometry_publisher.cpp
 * 
 * Dead reckoning одометрия для дифференциального привода.
 * 
 * Архитектура:
 *   Подписчик /cmd_vel (Twist) -> интегрирование Эйлера -> публикация /odom + TF
 * 
 * Интегрирование Эйлера (первый порядок):
 *   x_{k+1} = x_k + v_k * cos(theta_k) * dt
 *   y_{k+1} = y_k + v_k * sin(theta_k) * dt
 *   theta_{k+1} = theta_k + omega_k * dt
 * 
 * Почему Эйлер, а не Рунге-Кутта:
 *   - Частота 10 Гц, dt = 0.1 с. При v < 0.5 м/с погрешность Эйлера < 1%.
 *   - Рунге-Кутта 4-го порядка дал бы точнее, но требует 4 вычисления за шаг.
 *   - Для open-loop одометрии без энкодеров главный источник ошибки — не метод
 *     интегрирования, а несоответствие между командной скоростью и реальной.
 * 
 * Нормализация угла [-pi, pi]:
 *   - atan2 и кватернионы ожидают угол в этом диапазоне.
 *   - Без нормализации theta накапливает обороты: после 10 кругов theta = 62.8 рад.
 *   - Nav2 и AMCL сломаются при |theta| > pi.
 * 
 * TF и Odometry публикуются синхронно:
 *   - Одно и то же время (stamp) гарантирует, что RViz и алгоритмы видят
 *     согласованную картину. Рассинхрон = «прыжки» робота в RViz.
 * 
 * Covariance (пока нулевая):
 *   - В production заполняется из модели шума энкодеров.
 *   - Нулевая ковариация = «мы абсолютно уверены в позиции». Это ложь,
 *     но acceptable для MVP. TODO: заполнить после калибровки.
 */

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2/LinearMath/Quaternion.h>
#include <cmath>

class OdometryPublisher : public rclcpp::Node
{
public:
    OdometryPublisher() : Node("odometry_publisher")
    {
        // --- Параметры ROS2 ---
        // wheel_base: расстояние между центрами колёс (м). Должно совпадать с
        // WHEEL_BASE в прошивке ESP32 и с wheel_base в URDF/Xacro.
        // Несоответствие = Nav2 планирует для одного робота, а реальный едет иначе.
        this->declare_parameter<double>("wheel_base", 0.16);
        
        // max_speed: максимальная линейная скорость (м/с). Используется для
        // sanity-check входных данных. Если /cmd_vel пришёл с v=100 м/с — игнорируем.
        this->declare_parameter<double>("max_speed", 0.5);
        
        // publish_rate: частота публикации одометрии (Гц). 10 Гц = стандарт ROS2.
        // Меньше — RViz дергается. Больше — бесполезно без энкодеров.
        this->declare_parameter<double>("publish_rate", 10.0);

        wheel_base_ = this->get_parameter("wheel_base").as_double();
        max_speed_ = this->get_parameter("max_speed").as_double();
        double publish_rate = this->get_parameter("publish_rate").as_double();

        // --- Подписчик на /cmd_vel ---
        // Источник: teleop (keyboard) или Nav2 (autonomous).
        // Twist.linear.x = v (м/с), Twist.angular.z = omega (рад/с).
        cmd_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
            "/cmd_vel", 10,
            std::bind(&OdometryPublisher::cmdVelCallback, this, std::placeholders::_1));

        // --- Публикатор /odom ---
        // nav_msgs/Odometry содержит pose (положение) + twist (скорость).
        // twist дублирует /cmd_vel для удобства алгоритмов (не нужно подписываться на 2 топика).
        odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("/odom", 10);

        // --- TF Broadcaster ---
        // Публикует transform odom -> base_link.
        // RViz использует его для отображения robot model и лидара.
        tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

        // --- Таймер интегрирования ---
        // Период = 1 / publish_rate. Интегрирование происходит каждый тик таймера,
        // независимо от частоты прихода /cmd_vel. Это гарантирует стабильную частоту
        // публикации /odom (важно для SLAM Toolbox).
        auto period = std::chrono::milliseconds(static_cast<int>(1000.0 / publish_rate));
        timer_ = this->create_wall_timer(
            period,
            std::bind(&OdometryPublisher::updateOdometry, this));

        // --- Инициализация состояния ---
        // Начальная позиция: (0, 0, 0). Начало отсчёта odom-фрейма = точка включения робота.
        x_ = y_ = theta_ = 0.0;
        v_ = omega_ = 0.0;
        last_time_ = this->now();

        RCLCPP_INFO(this->get_logger(),
            "OdometryPublisher: wheel_base=%.2f m, max_speed=%.2f m/s, rate=%.1f Hz",
            wheel_base_, max_speed_, publish_rate);
    }

private:
    // --- Callback: запоминаем последнюю командную скорость ---
    // Не интегрируем здесь — интегрирование по таймеру даёт стабильный dt.
    void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
    {
        // Sanity check: если пришла crazy скорость (баг в Nav2 или teleop),
        // игнорируем, чтобы одометрия не улетела в бесконечность.
        double v = msg->linear.x;
        double omega = msg->angular.z;

        if (std::abs(v) > max_speed_ * 2.0) {
            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
                "Ignoring unrealistic linear velocity: %.2f (max %.2f)", v, max_speed_);
            return;
        }

        v_ = v;
        omega_ = omega;
    }

    // --- Интегрирование Эйлера + публикация ---
    void updateOdometry()
    {
        auto now = this->now();
        double dt = (now - last_time_).seconds();
        last_time_ = now;

        // Защита от dt <= 0 (возможно при скачках системного времени или pause/resume).
        if (dt <= 0.0) {
            return;
        }

        // Интегрирование позиции (локальная система координат робота).
        // v_ и omega_ — последние известные значения из /cmd_vel.
        // Предполагаем, что за dt скорость постоянна (zero-order hold).
        x_ += v_ * std::cos(theta_) * dt;
        y_ += v_ * std::sin(theta_) * dt;
        theta_ += omega_ * dt;

        // --- Нормализация угла в [-pi, pi] ---
        // Используем while вместо fmod, потому что fmod для отрицательных чисел
        // ведёт себя по-разному в разных стандартах. while — deterministic.
        while (theta_ > M_PI) {
            theta_ -= 2.0 * M_PI;
        }
        while (theta_ < -M_PI) {
            theta_ += 2.0 * M_PI;
        }

        // --- Кватернион из yaw (только вращение вокруг Z) ---
        // Для 2D-робота roll=0, pitch=0, yaw=theta.
        // Quaternion из угла Эйлера: qz = sin(yaw/2), qw = cos(yaw/2).
        tf2::Quaternion q;
        q.setRPY(0.0, 0.0, theta_);

        // --- Публикация nav_msgs/Odometry ---
        nav_msgs::msg::Odometry odom;
        odom.header.stamp = now;
        odom.header.frame_id = "odom";          // глобальный фрейм (начало отсчёта)
        odom.child_frame_id = "base_link";       // фрейм робота

        // Pose: положение и ориентация
        odom.pose.pose.position.x = x_;
        odom.pose.pose.position.y = y_;
        odom.pose.pose.position.z = 0.0;      // 2D-робот, z всегда 0

        odom.pose.pose.orientation.x = q.x();
        odom.pose.pose.orientation.y = q.y();
        odom.pose.pose.orientation.z = q.z();
        odom.pose.pose.orientation.w = q.w();

        // Twist: скорость в child_frame (base_link)
        // В дифференциальном приводе linear.x = v, angular.z = omega.
        // linear.y = 0 (нет движения вбок — non-holonomic constraint).
        odom.twist.twist.linear.x = v_;
        odom.twist.twist.linear.y = 0.0;
        odom.twist.twist.angular.z = omega_;

        // Covariance: пока нулевая (нет датчиков для оценки uncertainty).
        // TODO: после установки энкодеров заполнить из калибровочных данных.
        // 6x6 матрица для pose (x,y,z,roll,pitch,yaw) и 6x6 для twist.
        // Нули = "мы уверены на 100%". Это ложь, но acceptable для MVP.
        for (size_t i = 0; i < 36; ++i) {
            odom.pose.covariance[i] = 0.0;
            odom.twist.covariance[i] = 0.0;
        }
        // Диагональ pose: небольшая "подушка" безопасности, чтобы алгоритмы
        // не считали позицию абсолютно точной. 0.1 м² для x,y — reasonable.
        odom.pose.covariance[0] = 0.1;   // x variance
        odom.pose.covariance[7] = 0.1;   // y variance
        odom.pose.covariance[35] = 0.1;  // yaw variance

        odom_pub_->publish(odom);

        // --- Публикация TF odom -> base_link ---
        // Обязательно тот же stamp, что и в odom-сообщении.
        // Иначе RViz получит odom на t=0.1 и TF на t=0.15 — и покажет "прыжок".
        geometry_msgs::msg::TransformStamped tf;
        tf.header.stamp = now;
        tf.header.frame_id = "odom";
        tf.child_frame_id = "base_link";

        tf.transform.translation.x = x_;
        tf.transform.translation.y = y_;
        tf.transform.translation.z = 0.0;

        tf.transform.rotation.x = q.x();
        tf.transform.rotation.y = q.y();
        tf.transform.rotation.z = q.z();
        tf.transform.rotation.w = q.w();

        tf_broadcaster_->sendTransform(tf);

        // Отладочный вывод (throttled, чтобы не засорять лог).
        RCLCPP_DEBUG(this->get_logger(),
            "Odom: x=%.3f y=%.3f theta=%.3f v=%.3f w=%.3f dt=%.3f",
            x_, y_, theta_, v_, omega_, dt);
    }

    // --- Состояние ---
    double x_, y_, theta_;      // позиция (м, м, рад)
    double v_, omega_;           // текущая скорость (м/с, рад/с)
    double wheel_base_;          // L (м)
    double max_speed_;           // sanity check (м/с)
    rclcpp::Time last_time_;     // время предыдущего тика

    // --- ROS2 интерфейсы ---
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OdometryPublisher>());
    rclcpp::shutdown();
    return 0;
}
