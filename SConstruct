TARGET = 'polyd'

env = Environment()

files = []
files.append( Glob( '*.cpp' ) )

clean_list = []
clean_list.append( './sconsign.dblite' )
clean_list.append( 'core*' )
env.Clean( 'default', clean_list )

env.Append( CPPPATH = [ '/usr/include/GL' ] )

env.Append( CPPFLAGS = [ '-g' ] )
env.Append( CPPFLAGS = [ '-std=c++11' ] )

env.Append( LIBS = [ 'glut' ] )
env.Append( LIBS = [ 'GLU' ] )
env.Append( LIBS = [ 'GL' ] )

env.Program( TARGET, source = files )

