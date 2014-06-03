# vp-auditservice 0.0.0 Scons build script
#
# You must have "scons" installed to perform this build.
#
# This build script is subject to frequent changes.
#
#
import os
import os.path

env = Environment()


env.ParseConfig( 'xmlrpc-c-config c++2 client --cflags --libs' )

submodulesDirList = list()
submoduleDir = 'submodules'
for dirName, subdirList, fileList in os.walk(submoduleDir):
    submodulesDirList.append(dirName)

env.Append(CPPPATH = [submodulesDirList,'/usr/local/include/','src','src/crypto','src/parsers','src/parsers/jsoncpp','src/network','src/otlib/','src/queue','src/util'])
env.Append(LIBPATH = [submodulesDirList,'/usr/local/lib/'])
env.Append(LIBS = ['xmlrpc_client++','boost_system','boost_program_options','bmwrapper'])
#env.Append(CXXFLAGS = ['-std=c++11','-stdlib=libc++'])
env.Append(CXXFLAGS = ['-std=c++11'])



# This will be split apart as this project grows, for now this single scons file suffices.

sources = Split("""
src/parsers/MainConfigParser.cpp
src/parsers/jsoncpp/jsoncpp.cpp
src/network/Network.cpp
src/network/XmlRPC.cpp
src/crypto/base64.cpp
src/main.cpp
""")

object_list = env.Object(source = sources)

auditor = env.Program('vp-auditservice', source = object_list)
Default('vp-auditservice')


# Installation Target

env.Install("/usr/local/bin", 'vp-auditservice')
env.Alias('install', ['/usr/local/bin'])

