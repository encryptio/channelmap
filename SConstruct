import os

env = Environment( ENV = os.environ )
env.Append( CCFLAGS=['-Wall', '-O3', '-ffast-math', '-std=c99', '-g'] )
env.Append( CPPPATH=['.'] )

env.ParseConfig('pkg-config --cflags --libs sndfile')

env.Program("channelmap", ["main.c"])

