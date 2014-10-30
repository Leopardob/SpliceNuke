#
# Copyright 2010-2013 Fabric Engine Inc. All rights reserved.
#

import os, sys, platform, copy

Import('parentEnv', 'FABRIC_DIR', 'FABRIC_SPLICE_VERSION', 'STAGE_DIR', 'FABRIC_BUILD_OS', 'FABRIC_BUILD_TYPE', 'NUKE_INCLUDE_DIR', 'NUKE_LIB_DIR','NUKE_VERSION', 'sharedCapiFlags', 'spliceFlags', 'BOOST_DIR')

env = parentEnv.Clone()

nukeFlags = {
  'CPPPATH': [
      NUKE_INCLUDE_DIR
    ],
  'LIBPATH': [
    NUKE_LIB_DIR,
  ],
  'LIBS': [
    'DDImage'
  ]
}

if FABRIC_BUILD_OS == 'Windows':
  nukeFlags['CCFLAGS'] = ['/DNT_PLUGIN']
elif FABRIC_BUILD_OS == 'Linux':
  nukeFlags['CCFLAGS'] = ['-DLINUX']

env.MergeFlags(nukeFlags)
env.Append(CPPDEFINES = ["_SPLICE_NUKE_VERSION="+str(NUKE_VERSION[:4])])

env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)

target = 'spliceGeometry'

if FABRIC_BUILD_OS == 'Windows':
  target += '.dll'
if FABRIC_BUILD_OS == 'Linux':
  target += '.so'
if FABRIC_BUILD_OS == 'Darwin':
  target += '.dylib'

boostFlags = {
  'CPPPATH': [BOOST_DIR],
  'LIBPATH': [os.path.join(BOOST_DIR, 'lib')],
}
if FABRIC_BUILD_OS == 'Windows':
  if FABRIC_BUILD_TYPE == 'Debug':
    boostFlags['LIBS'] = [
      'libboost_thread-vc100-mt-sgd-1_55.lib',
      'libboost_system-vc100-mt-sgd-1_55.lib',
      'libboost_filesystem-vc100-mt-sgd-1_55.lib'
      ]
  else:
    boostFlags['LIBS'] = [
      'libboost_thread-vc100-mt-s-1_55.lib',
      'libboost_system-vc100-mt-s-1_55.lib',
      'libboost_filesystem-vc100-mt-s-1_55.lib'
      ]
else:
  boostFlags['LIBS'] = ['boost_thread','boost_system','boost_filesystem']
env.MergeFlags(boostFlags)

nukeModule = env.SharedLibrary(target = target, source = Glob('*.cpp'))

nukeFiles = []

installedModule = env.Install(STAGE_DIR, nukeModule)
nukeFiles.append(installedModule[0])

# also install the FabricCore dynamic library
if FABRIC_BUILD_OS == 'Linux':
  env.Append(LINKFLAGS = [Literal('-Wl,-rpath,$ORIGIN/../../../lib/')])
if FABRIC_BUILD_OS == 'Darwin':
  env.Append(LINKFLAGS = [Literal('-Wl,-rpath,@loader_path/../../..')])
if FABRIC_BUILD_OS == 'Windows':
  nukeFiles.append(env.Install(STAGE_DIR, env.Glob(os.path.join(FABRIC_DIR, 'lib', '*.dll'))))

# install PDB files on windows
if FABRIC_BUILD_TYPE == 'Debug' and FABRIC_BUILD_OS == 'Windows':
  env['CCPDBFLAGS']  = ['${(PDB and "/Fd%s_incremental.pdb /Zi" % File(PDB)) or ""}']
  pdbSource = nukeModule[0].get_abspath().rpartition('.')[0]+".pdb"
  pdbTarget = os.path.join(STAGE_DIR.abspath, os.path.split(pdbSource)[1])
  copyPdb = env.Command( 'copy', None, 'copy "%s" "%s" /Y' % (pdbSource, pdbTarget) )
  env.Depends( copyPdb, installedModule )
  env.AlwaysBuild(copyPdb)

alias = env.Alias('splicenuke', nukeFiles)
spliceData = (alias, nukeFiles)
Return('spliceData')
