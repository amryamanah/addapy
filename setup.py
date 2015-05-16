__author__ = 'Amry Fitra'

from setuptools import setup, find_packages
from codecs import open
from os import path

setup(
    name='addapy',
    version='0.1dev',
    license='GPL',
    long_description=open('README.rst').read(),
    packages=find_packages(),
    data_files=[
        ('addapy', ['addapy/addapy.pyd'])
    ],
)