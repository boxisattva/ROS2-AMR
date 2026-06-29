from setuptools import setup
from glob import glob  # <-- ДОБАВИТЬ ЭТО
import os
from setuptools import find_packages

package_name = 'amr_bringup'

setup(
    name=package_name,
    version='0.2.0',
    packages=find_packages(),
    data_files=[
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch', glob('launch/*.py')),  # <-- ТЕПЕРЬ РАБОТАЕТ
        ('share/' + package_name + '/config', glob('config/*')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Denis',
    maintainer_email='denis@example.com',
    description='AMR bringup package',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [],
    },
)
