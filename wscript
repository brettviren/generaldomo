#!/usr/bin/env waf
# encoding: utf-8

VERSION='0.0.0'
APPNAME='generaldomo'
top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_cxx')
    opt.load('waf_unit_test')

    opt.add_option('--with-cppzmq-include', type='string',
                   default=None,
                   help="give cppzmq include installation location")
    pass

def configure(cfg):
    cfg.env.CXXFLAGS += ['-std=c++17', '-g', '-O2']
    cfg.load('compiler_cxx')
    cfg.load('waf_unit_test')
    p = dict(mandatory=True, args='--cflags --libs')
    cfg.check_cfg(package='libzmq', uselib_store='ZMQ', **p);

    if cfg.options.with_cppzmq_include:
        cfg.env.INCLUDES_CPPZMQ = [cfg.options.with_cppzmq_include]

    cfg.check_cxx(header_name='zmq.hpp', uselib_store='CPPZMQ', use='ZMQ CPPZMQ'); 

    cfg.check(features='cxx cxxprogram', lib=['pthread'],
              uselib_store='PTHREAD')
    cfg.write_config_header('config.h')
    cfg.env['USES_LIB'] = ['ZMQ', 'CPPZMQ']
    cfg.env['USES_TEST'] = cfg.env['USES_LIB'] + ['PTHREAD']
    pass

from waflib.Configure import conf
@conf
def rpathify(bld, uses):
    rpath = [bld.env["PREFIX"] + '/lib', bld.out_dir]
    for u in uses:
        lpath = bld.env["LIBPATH_%s"%u]
        if not lpath:           # eg, system lib
            continue
        rpath.append(lpath[0])
    rpath = list(set(rpath))
    return rpath
    

def build(bld):
    uses = bld.env['USES_LIB']
    rpath = bld.rpathify(uses)

    # library
    sources = bld.path.ant_glob('src/*.cpp')
    bld.shlib(features='cxx',
              includes='inc',
              rpath=rpath,
              source = sources,
              target=APPNAME,
              uselib_store=APPNAME.upper(),
              use=uses)

    # executables
    mains = bld.path.ant_glob('apps/*.cpp')
    for main in mains:
        bld.program(features = 'cxx',
                    source = [main],
                    target = main.name.replace('.cpp',''),
                    install_path = None,
                    includes = ['inc'],
                    rpath = rpath,
                    use = [APPNAME] + uses)    

    # includes
    headers = bld.path.ant_glob("inc/"+APPNAME+"/*.hpp")
    bld.install_files('${PREFIX}/include/'+APPNAME, headers)

    # fake pkg-config
    bld(source='lib%s.pc.in'%APPNAME, VERSION=VERSION,
        LLIBS='-l'+APPNAME, REQUIRES='libzmq')
    # fake libtool
    lafile = bld.path.find_or_declare("lib%s.la"%APPNAME)
    bld(features='subst',
        source='lib%s.la.in'%APPNAME,
        target=lafile,
        **bld.env)
    bld.install_files('${PREFIX}/lib', lafile)
                      

    # testing
    uses = bld.env['USES_TEST']
    rpath = bld.rpathify(uses)
    tsources = bld.path.ant_glob('test/test*.cpp')
    for tmain in tsources:
        bld.program(features = 'test cxx',
                    source = [tmain],
                    target = tmain.name.replace('.cpp',''),
                    ut_cwd = bld.path,
                    install_path = None,
                    includes = ['inc','build','test'],
                    rpath = rpath,
                    use = uses)

    from waflib.Tools import waf_unit_test
    bld.add_post_fun(waf_unit_test.summary)
