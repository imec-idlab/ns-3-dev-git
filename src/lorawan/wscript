# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('lorawan', ['core', 'network', 'mobility', 'spectrum', 'propagation', 'applications']) # , 'visualizer'])
    module.source = [
        'model/lorawan.cc',
        'model/lorawan-enddevice-application.cc',
        'model/lorawan-error-model.cc',
        'model/lorawan-frame-header.cc',
        'model/lorawan-gateway-application.cc',
        'model/lorawan-interference-helper.cc',
        'model/lorawan-lqi-tag.cc',
        'model/lorawan-mac.cc',
        'model/lorawan-mac-header.cc',
        'model/lorawan-net-device.cc',
        'model/lorawan-phy.cc',
	'model/lorawan-spectrum-signal-parameters.cc',
	'model/lorawan-spectrum-value-helper.cc',
        'helper/lorawan-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('lorawan')
    module_test.source = [
        'test/lorawan-test-suite.cc',
        'test/lorawan-error-model-test.cc',
        'test/lorawan-retransmit-timeout-test.cc',
        'test/lorawan-packet-test.cc',
        'test/lorawan-phy-test.cc',
        'test/lorawan-ack-test.cc',
        'test/lorawan-gateway-forceoff-test.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'lorawan'
    headers.source = [
        'model/lorawan.h',
        'model/lorawan-enddevice-application.h',
        'model/lorawan-error-model.h',
        'model/lorawan-frame-header.h',
        'model/lorawan-gateway-application.h',
        'model/lorawan-interference-helper.h',
        'model/lorawan-lqi-tag.h',
        'model/lorawan-mac.h',
        'model/lorawan-mac-header.h',
        'model/lorawan-net-device.h',
        'model/lorawan-phy.h',
	'model/lorawan-spectrum-signal-parameters.h',
	'model/lorawan-spectrum-value-helper.h',
        'helper/lorawan-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

