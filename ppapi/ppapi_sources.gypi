# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'c_source_files': [
      'c/pp_array_output.h',
      'c/pp_bool.h',
      'c/pp_codecs.h',
      'c/pp_completion_callback.h',
      'c/pp_errors.h',
      'c/pp_file_info.h',
      'c/pp_graphics_3d.h',
      'c/pp_input_event.h',
      'c/pp_instance.h',
      'c/pp_macros.h',
      'c/pp_module.h',
      'c/pp_point.h',
      'c/pp_rect.h',
      'c/pp_resource.h',
      'c/pp_size.h',
      'c/pp_stdint.h',
      'c/pp_time.h',
      'c/pp_touch_point.h',
      'c/pp_var.h',
      'c/ppb.h',
      'c/ppb_audio.h',
      'c/ppb_audio_buffer.h',
      'c/ppb_audio_config.h',
      'c/ppb_compositor.h',
      'c/ppb_console.h',
      'c/ppb_core.h',
      'c/ppb_file_io.h',
      'c/ppb_file_mapping.h',
      'c/ppb_file_ref.h',
      'c/ppb_file_system.h',
      'c/ppb_fullscreen.h',
      'c/ppb_gamepad.h',
      'c/ppb_graphics_2d.h',
      'c/ppb_graphics_3d.h',
      'c/ppb_host_resolver.h',
      'c/ppb_image_data.h',
      'c/ppb_input_event.h',
      'c/ppb_instance.h',
      'c/ppb_media_stream_audio_track.h',
      'c/ppb_media_stream_video_track.h',
      'c/ppb_message_loop.h',
      'c/ppb_messaging.h',
      'c/ppb_mouse_cursor.h',
      'c/ppb_mouse_lock.h',
      'c/ppb_net_address.h',
      'c/ppb_network_list.h',
      'c/ppb_network_monitor.h',
      'c/ppb_network_proxy.h',
      'c/ppb_opengles2.h',
      'c/ppb_tcp_socket.h',
      'c/ppb_text_input_controller.h',
      'c/ppb_udp_socket.h',
      'c/ppb_url_loader.h',
      'c/ppb_url_request_info.h',
      'c/ppb_url_response_info.h',
      'c/ppb_var.h',
      'c/ppb_var_array.h',
      'c/ppb_var_array_buffer.h',
      'c/ppb_var_dictionary.h',
      'c/ppb_video_decoder.h',
      'c/ppb_video_frame.h',
      'c/ppb_view.h',
      'c/ppb_websocket.h',
      'c/ppp.h',
      'c/ppp_graphics_3d.h',
      'c/ppp_input_event.h',
      'c/ppp_instance.h',
      'c/ppp_messaging.h',
      'c/ppp_mouse_lock.h',

      # Dev interfaces.
      'c/dev/pp_cursor_type_dev.h',
      'c/dev/pp_optional_structs_dev.h',
      'c/dev/pp_video_dev.h',
      'c/dev/ppb_alarms_dev.h',
      'c/dev/ppb_buffer_dev.h',
      'c/dev/ppb_char_set_dev.h',
      'c/dev/ppb_cursor_control_dev.h',
      'c/dev/ppb_device_ref_dev.h',
      'c/dev/ppb_file_chooser_dev.h',
      'c/dev/ppb_font_dev.h',
      'c/dev/ppb_ime_input_event_dev.h',
      'c/dev/ppb_memory_dev.h',
      'c/dev/ppb_printing_dev.h',
      'c/dev/ppb_scrollbar_dev.h',
      'c/dev/ppb_text_input_dev.h',
      'c/dev/ppb_truetype_font_dev.h',
      'c/dev/ppb_url_util_dev.h',
      'c/dev/ppb_video_decoder_dev.h',
      'c/dev/ppb_widget_dev.h',
      'c/dev/ppb_zoom_dev.h',
      'c/dev/ppp_network_state_dev.h',
      'c/dev/ppp_scrollbar_dev.h',
      'c/dev/ppp_selection_dev.h',
      'c/dev/ppp_text_input_dev.h',
      'c/dev/ppp_video_decoder_dev.h',
      'c/dev/ppp_widget_dev.h',
      'c/dev/ppp_zoom_dev.h',

      # Private interfaces.
      'c/private/pp_file_handle.h',
      'c/private/pp_private_font_charset.h',
      'c/private/pp_video_frame_private.h',
      'c/private/ppb_content_decryptor_private.h',
      'c/private/ppb_ext_crx_file_system_private.h',
      'c/private/ppb_find_private.h',
      'c/private/ppb_flash.h',
      'c/private/ppb_flash_clipboard.h',
      'c/private/ppb_flash_file.h',
      'c/private/ppb_flash_font_file.h',
      'c/private/ppb_flash_fullscreen.h',
      'c/private/ppb_flash_menu.h',
      'c/private/ppb_flash_message_loop.h',
      'c/private/ppb_host_resolver_private.h',
      'c/private/ppb_input_event_private.h',
      'c/private/ppb_instance_private.h',
      'c/private/ppb_isolated_file_system_private.h',
      'c/private/ppb_nacl_private.h',
      'c/private/ppb_net_address_private.h',
      'c/private/ppb_output_protection_private.h',
      'c/private/ppb_pdf.h',
      'c/private/ppb_platform_verification_private.h',
      'c/private/ppb_proxy_private.h',
      'c/private/ppb_testing_private.h',
      'c/private/ppb_tcp_server_socket_private.h',
      'c/private/ppb_tcp_socket_private.h',
      'c/private/ppb_udp_socket_private.h',
      'c/private/ppb_uma_private.h',
      'c/private/ppb_video_destination_private.h',
      'c/private/ppb_video_source_private.h',
      'c/private/ppb_x509_certificate_private.h',
      'c/private/ppp_content_decryptor_private.h',
      'c/private/ppp_find_private.h',
      'c/private/ppp_instance_private.h',

      # Deprecated interfaces.
      'c/dev/deprecated_bool.h',
      'c/dev/ppb_var_deprecated.h',
      'c/dev/ppp_class_deprecated.h',

      # Trusted interfaces.
      'c/trusted/ppb_broker_trusted.h',
      'c/trusted/ppb_browser_font_trusted.h',
      'c/trusted/ppb_file_chooser_trusted.h',
      'c/trusted/ppb_url_loader_trusted.h',
      'c/trusted/ppp_broker.h',
    ],
    'cpp_source_files': [
      'cpp/array_output.cc',
      'cpp/array_output.h',
      'cpp/audio.cc',
      'cpp/audio.h',
      'cpp/audio_buffer.cc',
      'cpp/audio_buffer.h',
      'cpp/audio_config.cc',
      'cpp/audio_config.h',
      'cpp/completion_callback.h',
      'cpp/compositor.cc',
      'cpp/compositor.h',
      'cpp/compositor_layer.cc',
      'cpp/compositor_layer.h',
      'cpp/core.cc',
      'cpp/core.h',
      'cpp/directory_entry.cc',
      'cpp/directory_entry.h',
      'cpp/file_io.cc',
      'cpp/file_io.h',
      'cpp/file_ref.cc',
      'cpp/file_ref.h',
      'cpp/file_system.cc',
      'cpp/file_system.h',
      'cpp/fullscreen.cc',
      'cpp/fullscreen.h',
      'cpp/graphics_2d.cc',
      'cpp/graphics_2d.h',
      'cpp/graphics_3d.cc',
      'cpp/graphics_3d.h',
      'cpp/graphics_3d_client.cc',
      'cpp/graphics_3d_client.h',
      'cpp/host_resolver.cc',
      'cpp/host_resolver.h',
      'cpp/image_data.cc',
      'cpp/image_data.h',
      'cpp/input_event.cc',
      'cpp/input_event.h',
      'cpp/instance.cc',
      'cpp/instance.h',
      'cpp/instance_handle.cc',
      'cpp/instance_handle.h',
      'cpp/logging.h',
      'cpp/media_stream_audio_track.cc',
      'cpp/media_stream_audio_track.h',
      'cpp/media_stream_video_track.cc',
      'cpp/media_stream_video_track.h',
      'cpp/message_loop.cc',
      'cpp/message_loop.h',
      'cpp/module.cc',
      'cpp/module.h',
      'cpp/module_impl.h',
      'cpp/mouse_cursor.cc',
      'cpp/mouse_cursor.h',
      'cpp/mouse_lock.cc',
      'cpp/mouse_lock.h',
      'cpp/net_address.cc',
      'cpp/net_address.h',
      'cpp/network_list.cc',
      'cpp/network_list.h',
      'cpp/network_monitor.cc',
      'cpp/network_monitor.h',
      'cpp/network_proxy.cc',
      'cpp/network_proxy.h',
      'cpp/output_traits.h',
      'cpp/point.h',
      'cpp/rect.cc',
      'cpp/rect.h',
      'cpp/resource.cc',
      'cpp/resource.h',
      'cpp/size.h',
      'cpp/tcp_socket.cc',
      'cpp/tcp_socket.h',
      'cpp/text_input_controller.cc',
      'cpp/text_input_controller.h',
      'cpp/touch_point.h',
      'cpp/udp_socket.cc',
      'cpp/udp_socket.h',
      'cpp/url_loader.cc',
      'cpp/url_loader.h',
      'cpp/url_request_info.cc',
      'cpp/url_request_info.h',
      'cpp/url_response_info.cc',
      'cpp/url_response_info.h',
      'cpp/var.cc',
      'cpp/var.h',
      'cpp/var_array.cc',
      'cpp/var_array.h',
      'cpp/var_array_buffer.cc',
      'cpp/var_array_buffer.h',
      'cpp/var_dictionary.cc',
      'cpp/var_dictionary.h',
      'cpp/video_decoder.cc',
      'cpp/video_decoder.h',
      'cpp/video_frame.cc',
      'cpp/video_frame.h',
      'cpp/view.cc',
      'cpp/view.h',
      'cpp/websocket.cc',
      'cpp/websocket.h',

      # Dev interfaces.
      'cpp/dev/alarms_dev.cc',
      'cpp/dev/alarms_dev.h',
      'cpp/dev/array_dev.h',
      'cpp/dev/audio_input_dev.cc',
      'cpp/dev/audio_input_dev.h',
      'cpp/dev/buffer_dev.cc',
      'cpp/dev/buffer_dev.h',
      'cpp/dev/crypto_dev.cc',
      'cpp/dev/crypto_dev.h',
      'cpp/dev/cursor_control_dev.cc',
      'cpp/dev/cursor_control_dev.h',
      'cpp/dev/device_ref_dev.cc',
      'cpp/dev/device_ref_dev.h',
      'cpp/dev/file_chooser_dev.cc',
      'cpp/dev/file_chooser_dev.h',
      'cpp/dev/font_dev.cc',
      'cpp/dev/font_dev.h',
      'cpp/dev/ime_input_event_dev.cc',
      'cpp/dev/ime_input_event_dev.h',
      'cpp/dev/may_own_ptr_dev.h',
      'cpp/dev/memory_dev.cc',
      'cpp/dev/memory_dev.h',
      'cpp/dev/optional_dev.h',
      'cpp/dev/printing_dev.cc',
      'cpp/dev/printing_dev.h',
      'cpp/dev/scrollbar_dev.cc',
      'cpp/dev/scrollbar_dev.h',
      'cpp/dev/selection_dev.cc',
      'cpp/dev/selection_dev.h',
      'cpp/dev/string_wrapper_dev.cc',
      'cpp/dev/string_wrapper_dev.h',
      'cpp/dev/struct_wrapper_output_traits_dev.h',
      'cpp/dev/text_input_dev.cc',
      'cpp/dev/text_input_dev.h',
      'cpp/dev/to_c_type_converter_dev.h',
      'cpp/dev/truetype_font_dev.cc',
      'cpp/dev/truetype_font_dev.h',
      'cpp/dev/url_util_dev.cc',
      'cpp/dev/url_util_dev.h',
      'cpp/dev/video_capture_client_dev.cc',
      'cpp/dev/video_capture_client_dev.h',
      'cpp/dev/video_capture_dev.cc',
      'cpp/dev/video_capture_dev.h',
      'cpp/dev/video_decoder_client_dev.cc',
      'cpp/dev/video_decoder_client_dev.h',
      'cpp/dev/video_decoder_dev.cc',
      'cpp/dev/video_decoder_dev.h',
      'cpp/dev/view_dev.cc',
      'cpp/dev/view_dev.h',
      'cpp/dev/widget_client_dev.cc',
      'cpp/dev/widget_client_dev.h',
      'cpp/dev/widget_dev.cc',
      'cpp/dev/widget_dev.h',
      'cpp/dev/zoom_dev.cc',
      'cpp/dev/zoom_dev.h',

      # Deprecated interfaces.
      'cpp/dev/scriptable_object_deprecated.h',
      'cpp/dev/scriptable_object_deprecated.cc',

      # Private interfaces.
      'cpp/private/content_decryptor_private.cc',
      'cpp/private/content_decryptor_private.h',
      'cpp/private/ext_crx_file_system_private.cc',
      'cpp/private/ext_crx_file_system_private.h',
      'cpp/private/file_io_private.cc',
      'cpp/private/file_io_private.h',
      'cpp/private/find_private.cc',
      'cpp/private/find_private.h',
      'cpp/private/flash.cc',
      'cpp/private/flash.h',
      'cpp/private/flash_clipboard.cc',
      'cpp/private/flash_clipboard.h',
      'cpp/private/flash_device_id.cc',
      'cpp/private/flash_device_id.h',
      'cpp/private/flash_drm.cc',
      'cpp/private/flash_drm.h',
      'cpp/private/flash_file.cc',
      'cpp/private/flash_file.h',
      'cpp/private/flash_font_file.cc',
      'cpp/private/flash_font_file.h',
      'cpp/private/flash_fullscreen.cc',
      'cpp/private/flash_fullscreen.h',
      'cpp/private/flash_menu.cc',
      'cpp/private/flash_menu.h',
      'cpp/private/flash_message_loop.cc',
      'cpp/private/flash_message_loop.h',
      'cpp/private/host_resolver_private.cc',
      'cpp/private/host_resolver_private.h',
      'cpp/private/input_event_private.cc',
      'cpp/private/input_event_private.h',
      'cpp/private/instance_private.cc',
      'cpp/private/instance_private.h',
      'cpp/private/isolated_file_system_private.cc',
      'cpp/private/isolated_file_system_private.h',
      'cpp/private/net_address_private.cc',
      'cpp/private/net_address_private.h',
      'cpp/private/output_protection_private.cc',
      'cpp/private/output_protection_private.h',
      'cpp/private/pass_file_handle.cc',
      'cpp/private/pass_file_handle.h',
      'cpp/private/pdf.cc',
      'cpp/private/pdf.h',
      'cpp/private/platform_verification.cc',
      'cpp/private/platform_verification.h',
      'cpp/private/tcp_server_socket_private.cc',
      'cpp/private/tcp_server_socket_private.h',
      'cpp/private/tcp_socket_private.cc',
      'cpp/private/tcp_socket_private.h',
      'cpp/private/udp_socket_private.cc',
      'cpp/private/udp_socket_private.h',
      'cpp/private/uma_private.cc',
      'cpp/private/uma_private.h',
      'cpp/private/var_private.cc',
      'cpp/private/var_private.h',
      'cpp/private/video_destination_private.cc',
      'cpp/private/video_destination_private.h',
      'cpp/private/video_frame_private.cc',
      'cpp/private/video_frame_private.h',
      'cpp/private/video_source_private.cc',
      'cpp/private/video_source_private.h',
      'cpp/private/x509_certificate_private.cc',
      'cpp/private/x509_certificate_private.h',

      # Trusted interfaces.
      'cpp/trusted/browser_font_trusted.cc',
      'cpp/trusted/browser_font_trusted.h',
      'cpp/trusted/file_chooser_trusted.cc',
      'cpp/trusted/file_chooser_trusted.h',

      # Utility sources.
      'utility/completion_callback_factory.h',
      'utility/completion_callback_factory_thread_traits.h',
      'utility/graphics/paint_aggregator.cc',
      'utility/graphics/paint_aggregator.h',
      'utility/graphics/paint_manager.cc',
      'utility/graphics/paint_manager.h',
      'utility/threading/lock.cc',
      'utility/threading/lock.h',
      'utility/threading/simple_thread.cc',
      'utility/threading/simple_thread.h',
      'utility/websocket/websocket_api.cc',
      'utility/websocket/websocket_api.h',
    ],
    #
    # Common Testing source for trusted and untrusted (NaCl) pugins.
    #
    'test_common_source_files': [
      # Common test files
      'lib/gl/gles2/gles2.c',
      'lib/gl/gles2/gl2ext_ppapi.c',
      'lib/gl/gles2/gl2ext_ppapi.h',
      'tests/all_c_includes.h',
      'tests/all_cpp_includes.h',
      'tests/arch_dependent_sizes_32.h',
      'tests/arch_dependent_sizes_64.h',
      'tests/pp_thread.h',
      'tests/test_audio.cc',
      'tests/test_audio.h',
      'tests/test_audio_config.cc',
      'tests/test_audio_config.h',
      'tests/test_case.cc',
      'tests/test_case.h',
      'tests/test_console.cc',
      'tests/test_console.h',
      'tests/test_core.cc',
      'tests/test_core.h',
      'tests/test_cursor_control.cc',
      'tests/test_cursor_control.h',
      'tests/test_empty.cc',
      'tests/test_empty.h',
      'tests/test_file_io.cc',
      'tests/test_file_io.h',
      'tests/test_file_mapping.cc',
      'tests/test_file_mapping.h',
      'tests/test_file_ref.cc',
      'tests/test_file_ref.h',
      'tests/test_file_system.cc',
      'tests/test_file_system.h',
      'tests/test_fullscreen.cc',
      'tests/test_fullscreen.h',
      'tests/test_graphics_2d.cc',
      'tests/test_graphics_2d.h',
      'tests/test_graphics_3d.cc',
      'tests/test_graphics_3d.h',
      'tests/test_host_resolver.cc',
      'tests/test_host_resolver.h',
      'tests/test_host_resolver_private.cc',
      'tests/test_host_resolver_private.h',
      'tests/test_host_resolver_private_disallowed.cc',
      'tests/test_host_resolver_private_disallowed.h',
      'tests/test_image_data.cc',
      'tests/test_image_data.h',
      'tests/test_ime_input_event.cc',
      'tests/test_ime_input_event.h',
      'tests/test_input_event.cc',
      'tests/test_input_event.h',
      'tests/test_media_stream_audio_track.cc',
      'tests/test_media_stream_audio_track.h',
      'tests/test_media_stream_video_track.cc',
      'tests/test_media_stream_video_track.h',
      'tests/test_memory.cc',
      'tests/test_memory.h',
      'tests/test_message_loop.cc',
      'tests/test_message_loop.h',
      'tests/test_mouse_cursor.cc',
      'tests/test_mouse_cursor.h',
      'tests/test_mouse_lock.cc',
      'tests/test_mouse_lock.h',
      'tests/test_net_address.cc',
      'tests/test_net_address.h',
      'tests/test_net_address_private_untrusted.cc',
      'tests/test_net_address_private_untrusted.h',
      'tests/test_network_monitor.cc',
      'tests/test_network_monitor.h',
      'tests/test_network_proxy.cc',
      'tests/test_network_proxy.h',
      'tests/test_output_protection_private.cc',
      'tests/test_output_protection_private.h',
      'tests/test_paint_aggregator.cc',
      'tests/test_paint_aggregator.h',
      'tests/test_post_message.cc',
      'tests/test_post_message.h',
      'tests/test_printing.cc',
      'tests/test_printing.h',
      'tests/test_scrollbar.cc',
      'tests/test_scrollbar.h',
      'tests/test_tcp_server_socket_private.cc',
      'tests/test_tcp_server_socket_private.h',
      'tests/test_tcp_socket.cc',
      'tests/test_tcp_socket.h',
      'tests/test_tcp_socket_private.cc',
      'tests/test_tcp_socket_private.h',
      'tests/test_test_internals.cc',
      'tests/test_test_internals.h',
      'tests/test_trace_event.cc',
      'tests/test_trace_event.h',
      'tests/test_truetype_font.cc',
      'tests/test_truetype_font.h',
      'tests/test_udp_socket.cc',
      'tests/test_udp_socket.h',
      'tests/test_udp_socket_private.cc',
      'tests/test_udp_socket_private.h',
      'tests/test_uma.cc',
      'tests/test_uma.h',
      'tests/test_url_loader.cc',
      'tests/test_url_loader.h',
      'tests/test_url_request.cc',
      'tests/test_url_request.h',
      'tests/test_utils.cc',
      'tests/test_var.cc',
      'tests/test_var.h',
      'tests/test_var_resource.cc',
      'tests/test_var_resource.h',
      'tests/test_video_decoder.cc',
      'tests/test_video_decoder.h',
      'tests/test_video_destination.cc',
      'tests/test_video_destination.h',
      'tests/test_video_source.cc',
      'tests/test_video_source.h',
      'tests/test_view.cc',
      'tests/test_view.h',
      'tests/test_websocket.cc',
      'tests/test_websocket.h',
      'tests/testing_instance.cc',
      'tests/testing_instance.h',

      # Compile-time tests
      'tests/test_c_includes.c',
      'tests/test_cpp_includes.cc',
      'tests/test_struct_sizes.c',
     ],
    #
    # Sources used in NaCl tests.
    #
    'test_nacl_source_files': [
      # Test cases (PLEASE KEEP THIS SECTION IN ALPHABETICAL ORDER)
      'tests/test_tcp_server_socket_private_disallowed.cc',
      'tests/test_tcp_socket_private_disallowed.cc',
      'tests/test_udp_socket_private_disallowed.cc',
    ],
    #
    # Sources used in trusted tests.
    #
    'test_trusted_source_files': [
      # Test cases (PLEASE KEEP THIS SECTION IN ALPHABETICAL ORDER)
      'tests/test_broker.cc',
      'tests/test_broker.h',
      'tests/test_browser_font.cc',
      'tests/test_browser_font.h',
      'tests/test_buffer.cc',
      'tests/test_buffer.h',
      'tests/test_char_set.cc',
      'tests/test_char_set.h',
      'tests/test_crypto.cc',
      'tests/test_crypto.h',
      'tests/test_flash.cc',
      'tests/test_flash.h',
      'tests/test_flash_clipboard.cc',
      'tests/test_flash_clipboard.h',
      'tests/test_flash_drm.cc',
      'tests/test_flash_drm.h',
      'tests/test_flash_file.cc',
      'tests/test_flash_file.h',
      'tests/test_flash_fullscreen.cc',
      'tests/test_flash_fullscreen.h',
      'tests/test_flash_message_loop.cc',
      'tests/test_flash_message_loop.h',
      'tests/test_net_address_private.cc',
      'tests/test_net_address_private.h',
      'tests/test_pdf.cc',
      'tests/test_pdf.h',
      'tests/test_platform_verification_private.cc',
      'tests/test_platform_verification_private.h',
      'tests/test_talk_private.cc',
      'tests/test_talk_private.h',
      'tests/test_tcp_socket_private_trusted.cc',
      'tests/test_tcp_socket_private_trusted.h',
      'tests/test_url_util.cc',
      'tests/test_url_util.h',
      'tests/test_utils.h',
      'tests/test_video_decoder_dev.cc',
      'tests/test_video_decoder_dev.h',
      'tests/test_x509_certificate_private.cc',
      'tests/test_x509_certificate_private.h',

      # Deprecated test cases.
      'tests/test_instance_deprecated.cc',
      'tests/test_instance_deprecated.h',
      'tests/test_var_deprecated.cc',
      'tests/test_var_deprecated.h',
    ],
  },
}
