
import sys
import os

sys.path.append(os.environ['HELLOSAYER_PYTHON_PATH'])

def header(msg, width=100, char='-'):
    print('\n'+f' {msg} '.center(width, char)+'\n')

wd = 100
header('HelloSayerLibPythonTests.py trying to load HelloSayerLib')
header('Using directory specified with system env var \'HELLOSAYER_PYTHON_PATH\'')

import HelloSayerLib as hsl

header('HelloSayerLib loaded from .pyd... Starting Tests')

hs = hsl.HelloSayer()
message = hs.sayHello()

print(f'hs.sayHello() said:\n  "{message}"') 
print(f'\nHelloSayerLib instance docstring is:\n  {hsl.__doc__}')

header('Tests Complete')