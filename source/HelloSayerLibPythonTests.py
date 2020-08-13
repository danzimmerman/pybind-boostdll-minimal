
import sys

sys.path.append(r'C:\Code\dan\minimal_examples\HelloSayer\python')
import HelloSayerLib as hsl

wd = 100

def header(msg, width=100, char='-'):
    print('\n'+f' {msg} '.center(width, char)+'\n')

header('HelloSayerLib loaded from .pyd... Starting Tests')

hs = hsl.HelloSayer()
message = hs.sayHello()

print(f'hs.sayHello() said:\n  "{message}"') 
print(f'\nHelloSayerLib instance docstring is:\n  {hsl.__doc__}')

header('Tests Complete')