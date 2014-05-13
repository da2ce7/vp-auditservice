# Auditor 0.0.0 Scons build script
#
# You must have "scons" installed to perform this build.
#
# This build script is subject to frequent changes.
#
#


env = Environment()


env.ParseConfig( 'xmlrpc-c-config c++2 client --cflags --libs' )
env.Append(CPPPATH = ['/usr/local/include/','src','src/crypto','src/parsers','src/parsers/jsoncpp','src/network','src/queue'])
env.Append(LIBPATH = ['/usr/local/lib/'])
env.Append(LIBS = ['xmlrpc_client++','boost_system','boost_program_options'])
#env.Append(CXXFLAGS = ['-std=c++11','-stdlib=libc++'])
env.Append(CXXFLAGS = ['-std=c++11'])



# This will be split apart as this project grows, for now this single scons file suffices.

sources = Split("""
src/parsers/MainConfigParser.cpp
src/parsers/jsoncpp/jsoncpp.cpp
src/queue/BitMessageQueue.cpp
src/network/Network.cpp
src/network/BitMessage.cpp
src/network/XmlRPC.cpp
src/crypto/base64.cpp
src/main.cpp
""")

object_list = env.Object(source = sources)

auditor = env.Program('auditor', source = object_list)
Default(auditor)


# Installation Target

env.Install("/usr/local/bin", auditor)
env.Alias('install', ['/usr/local/bin'])

