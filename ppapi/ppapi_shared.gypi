# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'ppapi_shared',
      'type': '<(component)',
      'dependencies': [
        'ppapi.gyp:ppapi_c',
        '../base/base.gyp:base',
        '../base/base.gyp:base_i18n',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../build/temp_gyp/googleurl.gyp:googleurl',
        '../gpu/gpu.gyp:gles2_implementation',
        '../skia/skia.gyp:skia',
        '../third_party/icu/icu.gyp:icuuc',
        '../ui/gfx/surface/surface.gyp:surface',
      ],
      'defines': [
        'PPAPI_SHARED_IMPLEMENTATION',
        'PPAPI_THUNK_IMPLEMENTATION',
      ],
      'include_dirs': [
        '..',
      ],
      'export_dependent_settings': [
        '../base/base.gyp:base',
      ],
      'sources': [
        'shared_impl/audio_config_impl.cc',
        'shared_impl/audio_config_impl.h',
        'shared_impl/audio_impl.cc',
        'shared_impl/audio_impl.h',
        'shared_impl/char_set_impl.cc',
        'shared_impl/char_set_impl.h',
        'shared_impl/crypto_impl.cc',
        'shared_impl/crypto_impl.h',
        'shared_impl/file_ref_impl.cc',
        'shared_impl/file_ref_impl.h',
        'shared_impl/font_impl.cc',
        'shared_impl/font_impl.h',
        'shared_impl/function_group_base.cc',
        'shared_impl/function_group_base.h',
        'shared_impl/graphics_3d_impl.cc',
        'shared_impl/graphics_3d_impl.h',
        'shared_impl/host_resource.h',
        'shared_impl/id_assignment.cc',
        'shared_impl/id_assignment.h',
        'shared_impl/image_data_impl.cc',
        'shared_impl/image_data_impl.h',
        'shared_impl/input_event_impl.cc',
        'shared_impl/input_event_impl.h',
        'shared_impl/instance_impl.cc',
        'shared_impl/instance_impl.h',
        'shared_impl/opengles2_impl.cc',
        'shared_impl/opengles2_impl.h',
        'shared_impl/ppapi_preferences.cc',
        'shared_impl/ppapi_preferences.h',
        'shared_impl/ppp_instance_combined.cc',
        'shared_impl/ppp_instance_combined.h',
        'shared_impl/resource.cc',
        'shared_impl/resource.h',
        'shared_impl/resource_tracker.cc',
        'shared_impl/resource_tracker.h',
        'shared_impl/scoped_pp_resource.cc',
        'shared_impl/scoped_pp_resource.h',
        'shared_impl/time_conversion.cc',
        'shared_impl/time_conversion.h',
        'shared_impl/tracker_base.cc',
        'shared_impl/tracker_base.h',
        'shared_impl/url_request_info_impl.cc',
        'shared_impl/url_request_info_impl.h',
        'shared_impl/url_util_impl.cc',
        'shared_impl/url_util_impl.h',
        'shared_impl/var.cc',
        'shared_impl/var.h',
        'shared_impl/var_tracker.cc',
        'shared_impl/var_tracker.h',
        'shared_impl/video_decoder_impl.cc',
        'shared_impl/video_decoder_impl.h',
        'shared_impl/webkit_forwarding.cc',
        'shared_impl/webkit_forwarding.h',

        'thunk/common.h',
        'thunk/common.cc',
        'thunk/enter.h',
        'thunk/ppb_audio_api.h',
        'thunk/ppb_audio_config_api.h',
        'thunk/ppb_audio_config_thunk.cc',
        'thunk/ppb_audio_thunk.cc',
        'thunk/ppb_audio_trusted_thunk.cc',
        'thunk/ppb_broker_api.h',
        'thunk/ppb_broker_thunk.cc',
        'thunk/ppb_buffer_api.h',
        'thunk/ppb_buffer_thunk.cc',
        'thunk/ppb_buffer_trusted_api.h',
        'thunk/ppb_buffer_trusted_thunk.cc',
        'thunk/ppb_char_set_api.h',
        'thunk/ppb_char_set_thunk.cc',
        'thunk/ppb_context_3d_api.h',
        'thunk/ppb_context_3d_thunk.cc',
        'thunk/ppb_context_3d_trusted_thunk.cc',
        'thunk/ppb_cursor_control_api.h',
        'thunk/ppb_cursor_control_thunk.cc',
        'thunk/ppb_directory_reader_api.h',
        'thunk/ppb_directory_reader_thunk.cc',
        'thunk/ppb_input_event_api.h',
        'thunk/ppb_input_event_thunk.cc',
        'thunk/ppb_file_chooser_api.h',
        'thunk/ppb_file_chooser_thunk.cc',
        'thunk/ppb_file_io_api.h',
        'thunk/ppb_file_io_thunk.cc',
        'thunk/ppb_file_io_trusted_thunk.cc',
        'thunk/ppb_file_ref_api.h',
        'thunk/ppb_file_ref_thunk.cc',
        'thunk/ppb_file_system_api.h',
        'thunk/ppb_file_system_thunk.cc',
        'thunk/ppb_find_api.h',
        'thunk/ppb_find_thunk.cc',
        'thunk/ppb_flash_menu_api.h',
        'thunk/ppb_flash_menu_thunk.cc',
        'thunk/ppb_flash_net_connector_api.h',
        'thunk/ppb_flash_net_connector_thunk.cc',
        'thunk/ppb_flash_tcp_socket_api.h',
        'thunk/ppb_flash_tcp_socket_thunk.cc',
        'thunk/ppb_font_api.h',
        'thunk/ppb_font_thunk.cc',
        'thunk/ppb_fullscreen_thunk.cc',
        'thunk/ppb_gles_chromium_texture_mapping_thunk.cc',
        'thunk/ppb_graphics_2d_api.h',
        'thunk/ppb_graphics_2d_thunk.cc',
        'thunk/ppb_graphics_3d_api.h',
        'thunk/ppb_graphics_3d_thunk.cc',
        'thunk/ppb_graphics_3d_trusted_thunk.cc',
        'thunk/ppb_image_data_api.h',
        'thunk/ppb_image_data_thunk.cc',
        'thunk/ppb_image_data_trusted_thunk.cc',
        'thunk/ppb_instance_api.h',
        'thunk/ppb_instance_thunk.cc',
        'thunk/ppb_layer_compositor_api.h',
        'thunk/ppb_layer_compositor_thunk.cc',
        'thunk/ppb_messaging_thunk.cc',
        'thunk/ppb_pdf_api.h',
        'thunk/ppb_query_policy_thunk.cc',
        'thunk/ppb_scrollbar_api.h',
        'thunk/ppb_scrollbar_thunk.cc',
        'thunk/ppb_surface_3d_api.h',
        'thunk/ppb_surface_3d_thunk.cc',
        'thunk/ppb_transport_api.h',
        'thunk/ppb_transport_thunk.cc',
        'thunk/ppb_url_loader_api.h',
        'thunk/ppb_url_loader_thunk.cc',
        'thunk/ppb_url_request_info_api.h',
        'thunk/ppb_url_request_info_thunk.cc',
        'thunk/ppb_url_response_info_api.h',
        'thunk/ppb_url_response_info_thunk.cc',
        'thunk/ppb_video_capture_api.h',
        'thunk/ppb_video_capture_thunk.cc',
        'thunk/ppb_video_decoder_api.h',
        'thunk/ppb_video_decoder_thunk.cc',
        'thunk/ppb_video_layer_api.h',
        'thunk/ppb_video_layer_thunk.cc',
        'thunk/ppb_widget_api.h',
        'thunk/ppb_widget_thunk.cc',
        'thunk/ppb_zoom_thunk.cc',
        'thunk/thunk.h',
      ],
    },
  ],
}
