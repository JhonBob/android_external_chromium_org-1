# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'ppapi_proxy',
      'type': '<(component)',
      'dependencies': [
        '../base/base.gyp:base',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../gpu/gpu.gyp:gles2_implementation',
        '../gpu/gpu.gyp:gpu_ipc',
        '../ipc/ipc.gyp:ipc',
        '../skia/skia.gyp:skia',
        '../ui/gfx/surface/surface.gyp:surface',
        'ppapi.gyp:ppapi_c',
        'ppapi_shared',
      ],
      'all_dependent_settings': {
        'include_dirs': [
           '..',
        ],
      },
      'include_dirs': [
        '..',
        '../..',  # For nacl includes to work.
      ],
      'sources': [
        # Take some standalong files from the C++ wrapper allowing us to more
        # easily make async callbacks in the proxy. We can't depend on the
        # full C++ wrappers at this layer since the C++ wrappers expect
        # symbols defining the globals for "being a plugin" which we are not.
        # These callback files are standalone.
        'cpp/completion_callback.cc',
        'cpp/completion_callback.h',
        'utility/completion_callback_factory.h',

        'proxy/broker_dispatcher.cc',
        'proxy/broker_dispatcher.h',
        'proxy/dispatcher.cc',
        'proxy/dispatcher.h',
        'proxy/enter_proxy.h',
        'proxy/host_dispatcher.cc',
        'proxy/host_dispatcher.h',
        'proxy/host_var_serialization_rules.cc',
        'proxy/host_var_serialization_rules.h',
        'proxy/interface_list.cc',
        'proxy/interface_list.h',
        'proxy/interface_proxy.cc',
        'proxy/interface_proxy.h',
        'proxy/plugin_array_buffer_var.cc',
        'proxy/plugin_array_buffer_var.h',
        'proxy/plugin_dispatcher.cc',
        'proxy/plugin_dispatcher.h',
        'proxy/plugin_globals.cc',
        'proxy/plugin_globals.h',
        'proxy/plugin_message_filter.cc',
        'proxy/plugin_message_filter.h',
        'proxy/plugin_resource_tracker.cc',
        'proxy/plugin_resource_tracker.h',
        'proxy/plugin_var_serialization_rules.cc',
        'proxy/plugin_var_serialization_rules.h',
        'proxy/plugin_var_tracker.cc',
        'proxy/plugin_var_tracker.h',
        'proxy/ppapi_messages.cc',
        'proxy/ppapi_messages.h',
        'proxy/ppapi_command_buffer_proxy.h',
        'proxy/ppapi_command_buffer_proxy.cc',
        'proxy/ppapi_param_traits.cc',
        'proxy/ppapi_param_traits.h',
        'proxy/ppb_audio_input_proxy.cc',
        'proxy/ppb_audio_input_proxy.h',
        'proxy/ppb_audio_proxy.cc',
        'proxy/ppb_audio_proxy.h',
        'proxy/ppb_broker_proxy.cc',
        'proxy/ppb_broker_proxy.h',
        'proxy/ppb_buffer_proxy.cc',
        'proxy/ppb_buffer_proxy.h',
        'proxy/ppb_core_proxy.cc',
        'proxy/ppb_core_proxy.h',
        'proxy/ppb_file_chooser_proxy.cc',
        'proxy/ppb_file_chooser_proxy.h',
        'proxy/ppb_file_io_proxy.cc',
        'proxy/ppb_file_io_proxy.h',
        'proxy/ppb_file_ref_proxy.cc',
        'proxy/ppb_file_ref_proxy.h',
        'proxy/ppb_file_system_proxy.cc',
        'proxy/ppb_file_system_proxy.h',
        'proxy/ppb_flash_clipboard_proxy.cc',
        'proxy/ppb_flash_clipboard_proxy.h',
        'proxy/ppb_flash_file_proxy.cc',
        'proxy/ppb_flash_file_proxy.h',
        'proxy/ppb_flash_proxy.cc',
        'proxy/ppb_flash_proxy.h',
        'proxy/ppb_flash_menu_proxy.cc',
        'proxy/ppb_flash_menu_proxy.h',
        'proxy/ppb_flash_message_loop_proxy.cc',
        'proxy/ppb_flash_message_loop_proxy.h',
        'proxy/ppb_graphics_2d_proxy.cc',
        'proxy/ppb_graphics_2d_proxy.h',
        'proxy/ppb_graphics_3d_proxy.cc',
        'proxy/ppb_graphics_3d_proxy.h',
        'proxy/ppb_host_resolver_private_proxy.cc',
        'proxy/ppb_host_resolver_private_proxy.h',
        'proxy/ppb_image_data_proxy.cc',
        'proxy/ppb_image_data_proxy.h',
        'proxy/ppb_instance_proxy.cc',
        'proxy/ppb_instance_proxy.h',
        'proxy/ppb_message_loop_proxy.cc',
        'proxy/ppb_message_loop_proxy.h',
        'proxy/ppb_network_monitor_private_proxy.cc',
        'proxy/ppb_network_monitor_private_proxy.h',
        'proxy/ppb_pdf_proxy.cc',
        'proxy/ppb_pdf_proxy.h',
        'proxy/ppb_talk_private_proxy.cc',
        'proxy/ppb_talk_private_proxy.h',
        'proxy/ppb_tcp_server_socket_private_proxy.cc',
        'proxy/ppb_tcp_server_socket_private_proxy.h',
        'proxy/ppb_tcp_socket_private_proxy.cc',
        'proxy/ppb_tcp_socket_private_proxy.h',
        'proxy/ppb_testing_proxy.cc',
        'proxy/ppb_testing_proxy.h',
        'proxy/ppb_text_input_proxy.cc',
        'proxy/ppb_text_input_proxy.h',
        'proxy/ppb_udp_socket_private_proxy.cc',
        'proxy/ppb_udp_socket_private_proxy.h',
        'proxy/ppb_url_loader_proxy.cc',
        'proxy/ppb_url_loader_proxy.h',
        'proxy/ppb_url_response_info_proxy.cc',
        'proxy/ppb_url_response_info_proxy.h',
        'proxy/ppb_var_deprecated_proxy.cc',
        'proxy/ppb_var_deprecated_proxy.h',
        'proxy/ppb_video_capture_proxy.cc',
        'proxy/ppb_video_capture_proxy.h',
        'proxy/ppb_video_decoder_proxy.cc',
        'proxy/ppb_video_decoder_proxy.h',
        'proxy/ppb_x509_certificate_private_proxy.cc',
        'proxy/ppb_x509_certificate_private_proxy.h',
        'proxy/ppp_class_proxy.cc',
        'proxy/ppp_class_proxy.h',
        'proxy/ppp_graphics_3d_proxy.cc',
        'proxy/ppp_graphics_3d_proxy.h',
        'proxy/ppp_input_event_proxy.cc',
        'proxy/ppp_input_event_proxy.h',
        'proxy/ppp_instance_private_proxy.cc',
        'proxy/ppp_instance_private_proxy.h',
        'proxy/ppp_instance_proxy.cc',
        'proxy/ppp_instance_proxy.h',
        'proxy/ppp_messaging_proxy.cc',
        'proxy/ppp_messaging_proxy.h',
        'proxy/ppp_mouse_lock_proxy.cc',
        'proxy/ppp_mouse_lock_proxy.h',
        'proxy/ppp_printing_proxy.cc',
        'proxy/ppp_printing_proxy.h',
        'proxy/ppp_text_input_proxy.cc',
        'proxy/ppp_text_input_proxy.h',
        'proxy/ppp_video_decoder_proxy.cc',
        'proxy/ppp_video_decoder_proxy.h',
        'proxy/proxy_array_output.cc',
        'proxy/proxy_array_output.h',
        'proxy/proxy_channel.cc',
        'proxy/proxy_channel.h',
        'proxy/proxy_module.cc',
        'proxy/proxy_module.h',
        'proxy/proxy_object_var.cc',
        'proxy/proxy_object_var.h',
        'proxy/resource_creation_proxy.cc',
        'proxy/resource_creation_proxy.h',
        'proxy/serialized_flash_menu.cc',
        'proxy/serialized_flash_menu.h',
        'proxy/serialized_structs.cc',
        'proxy/serialized_structs.h',
        'proxy/serialized_var.cc',
        'proxy/serialized_var.h',
        'proxy/var_serialization_rules.h',
      ],
      'defines': [
        'PPAPI_PROXY_IMPLEMENTATION',
      ],
    },
  ],
}
