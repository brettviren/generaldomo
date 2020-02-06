import setuptools

setuptools.setup(
    name="generaldomo",
    version="0.0.0",
    author="Brett Viren",
    author_email="brett.viren@gmail.com",
    description="A slightly more general version of ZeroMQ Zguide Majordomo examples",
    url="https://brettviren.github.io/generaldomo",
    packages=setuptools.find_packages(),
    python_requires='>=3.6',
    install_requires = [
        "click",
        "pyzmq",
     ],
    entry_points = dict(
        console_scripts = [
            'generaldomo = generaldomo.__main__:main',
        ]
    ),
)
