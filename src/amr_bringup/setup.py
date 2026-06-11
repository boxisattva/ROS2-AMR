from setuptools import find_packages, setup

package_name = 'amr_bringup'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/amr_bringup/launch', ['launch/bringup.launch.py']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='denis',
    maintainer_email='boxisattva@yandex.ru',
    description='AMR bringup package: talker/listener demo',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'talker = amr_bringup.talker:main',
            'listener = amr_bringup.listener:main',
        ],
    },
)
