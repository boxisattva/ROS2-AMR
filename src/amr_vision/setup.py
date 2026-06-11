from setuptools import find_packages, setup

package_name = 'amr_vision'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/amr_vision/launch', ['launch/vision.launch.py']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='denis',
    maintainer_email='boxisattva@yandex.ru',
    description='AMR vision: camera and computer vision',
    license='MIT',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
    'console_scripts': [
        'camera_node = amr_vision.camera_node:main',
    ],
},
)
