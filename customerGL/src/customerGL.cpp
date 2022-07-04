
#include <screen/screen.h>
#include <iostream>
#include <thread>

auto create_window(screen_context_t screen_ctx) -> screen_window_t {
  screen_window_t screen_win;

  screen_create_window(&screen_win, screen_ctx);

  screen_set_window_property_iv(
      screen_win, SCREEN_PROPERTY_USAGE,
      (const int[]){SCREEN_USAGE_OPENGL_ES2 | SCREEN_USAGE_WRITE |
                    SCREEN_USAGE_NATIVE});

  int buffer_size[2] = {720, 720};
  screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_BUFFER_SIZE,
                                buffer_size);

  screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_FORMAT,
                                (const int[]){SCREEN_FORMAT_RGBA8888});
  int nbuffers = 2;
  screen_create_window_buffers(screen_win, nbuffers);

  return screen_win;
}

auto get_render_buffer(screen_window_t screen_win) -> screen_buffer_t {
  screen_buffer_t screen_wbuf[2] = {0};
  screen_get_window_property_pv(screen_win, SCREEN_PROPERTY_RENDER_BUFFERS,
                                (void **)&screen_wbuf);
  return screen_wbuf[0];
}

int main() {
  std::cout << "consumer\n";

  screen_context_t screen_ctx;
  screen_create_context(&screen_ctx, SCREEN_APPLICATION_CONTEXT | SCREEN_BUFFER_PROVIDER_CONTEXT);

  screen_stream_t stream_c;
  screen_create_stream(&stream_c, screen_ctx);

  // create a context
	char window_group_name[64];
	screen_window_t screen_win = 0;
	screen_create_window(&screen_win, screen_ctx);
	screen_get_window_property_cv(screen_win,SCREEN_PROPERTY_ID,sizeof(window_group_name),window_group_name);
	int format = SCREEN_FORMAT_RGBA8888;
	screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_FORMAT, &format);
	int usage = SCREEN_USAGE_NATIVE;
	screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_USAGE, &usage);
	//const int alpha =255;
	//screen_set_window_property_iv(screen_win,SCREEN_PROPERTY_GLOBAL_ALPHA, &alpha);
	int nbuffers = 1;
	screen_create_window_buffers(screen_win, nbuffers);


	screen_buffer_t screen_buf[2];
	screen_get_window_property_pv(screen_win, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)screen_buf);
	int backgroundColor = 0x22ffff00;
	int win_background[] = { SCREEN_BLIT_COLOR, backgroundColor, SCREEN_BLIT_END };
	screen_fill(screen_ctx, screen_buf[0], win_background);
	screen_post_window(screen_win, screen_buf[0], 0, NULL, 0);


	screen_window_t screen_child_window = 0;
	int wintype=SCREEN_CHILD_WINDOW;
	screen_create_window_type(&screen_child_window,screen_ctx,wintype);
	screen_join_window_group(screen_child_window,window_group_name);
	int size[2] = {640, 360};
	screen_set_window_property_iv(screen_child_window, SCREEN_PROPERTY_BUFFER_SIZE, size);
	screen_set_window_property_iv(screen_child_window, SCREEN_PROPERTY_SOURCE_SIZE, size);
	screen_set_window_property_iv(screen_child_window, SCREEN_PROPERTY_CLIP_POSITION, size);

	screen_create_window_buffers(screen_child_window, nbuffers);

	screen_get_window_property_pv(screen_child_window, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)screen_buf);
	win_background[1] = 0xff00ffffff;
	screen_fill(screen_ctx, screen_buf[0], win_background);
	screen_post_window(screen_child_window, screen_buf[0], 0, NULL, 0);



  // get producers' stream
  screen_event_t event;
  int stream_p_id = -1;
  screen_stream_t stream_p = NULL;
  screen_buffer_t
      acquired_buffer; /* Buffer that's been acquired from a stream */

  /* Create an event so that you can retrieve an event from Screen. */
  screen_create_event(&event);

  std::thread renderer;
  int rect[4] = {0,0,640,360};
  	int hg[] = {
  		SCREEN_BLIT_SOURCE_WIDTH, 640,
  		SCREEN_BLIT_SOURCE_HEIGHT, 360,
  		SCREEN_BLIT_DESTINATION_X, 0,
  		SCREEN_BLIT_DESTINATION_Y, 0,
  		SCREEN_BLIT_DESTINATION_WIDTH, 100,
  		SCREEN_BLIT_DESTINATION_HEIGHT, 100,
  		SCREEN_BLIT_TRANSPARENCY, SCREEN_TRANSPARENCY_SOURCE_OVER,
  		SCREEN_BLIT_END
  	};

  while (1) {
    std::cout << "consumer) waiting evetns\n";
    int event_type = SCREEN_EVENT_NONE;
    int object_type;

    /* Get event from Screen for the consumer's context. */
    screen_get_event(screen_ctx, event, -1);

    /* Get the type of event from the event retrieved. */
    screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &event_type);
    printf("%s:%d,!\n",__FUNCTION__,__LINE__);
    /* Process the event if it's a SCREEN_EVENT_CREATE event. */
    if (event_type == SCREEN_EVENT_CREATE) {
      std::cout << "consumer) SCREEN_EVENT_CREATE\n";
      /* Determine that this event is due to a producer stream granting
       * permissions. */
      screen_get_event_property_iv(event, SCREEN_PROPERTY_OBJECT_TYPE,
                                   &object_type);

      if (object_type == SCREEN_OBJECT_TYPE_STREAM) {
        /* Get the handle for the producer's stream from the event. */
        screen_get_event_property_pv(event, SCREEN_PROPERTY_STREAM,
                                     (void **)&stream_p);

        if (stream_p != NULL) {
        	printf("%s:%d,!\n",__FUNCTION__,__LINE__);
          /* Get the handle for the producer's stream ID from the event.
           * If there are multiple producers in the system, consumers can use
           * the producer's stream ID
           * as a way to verify whether the SCREEN_EVENT_CREATE event is from
           * the producer that the consumer
           * is expecting. In this example, we assume there's only one producer
           * in the system.
           */
          screen_get_stream_property_iv(stream_p, SCREEN_PROPERTY_ID,
                                        &stream_p_id);

          std::cout << "consumer) found producer : " << stream_p_id << "\n";

          // This function is used when the consumer of a stream is in a
          // different process. Establish the connection between the consumer's
          // stream and the producer's stream
          auto success =
              screen_consume_stream_buffers(stream_c,  // consumer
                                            0,  // num of the buffers shared
                                            stream_p);  // producer
          printf("%s:%d,!\n",__FUNCTION__,__LINE__);
          if (success == -1) {
            std::cout << " failed to consume the stream\n";
            return -1;
          }

          renderer = std::thread([&]() {
            screen_buffer_t sbuffer = nullptr;
            printf("%s:%d,!\n",__FUNCTION__,__LINE__);
            while (1) {
              // blocks until there's a front buffer available to acquire.
              // if don't block, SCREEN_ACQUIRE_DONT_BLOCK
              success = screen_acquire_buffer(&sbuffer, stream_c, nullptr,
                                              nullptr, nullptr, 0);
              printf("%s:%d,success=%d\n",__FUNCTION__,__LINE__,success);
              if (success == -1) {
                std::cout << "consumer) failed to acquired_buffer\n";
                break;
              }

              // get window buffer
              //auto wbuffer = get_render_buffer(screen_child_window);
              screen_buffer_t wbuffer[2] = {0};
              screen_get_window_property_pv(screen_child_window, SCREEN_PROPERTY_RENDER_BUFFERS,(void **)&wbuffer);
              if (wbuffer[0] != nullptr && sbuffer != NULL) {
            	  printf("%s:%d,wbuffer != nullptr and sbuffer != NULL\n",__FUNCTION__,__LINE__);
            	  screen_blit(screen_ctx, wbuffer[0], sbuffer, 0);
                //screen_blit(screen_ctx, wbuffer, sbuffer, hg);
                screen_post_window(screen_child_window, wbuffer[0], 0, nullptr, 0);
              }

              if (sbuffer != nullptr) {
                screen_release_buffer(sbuffer);
              }
            }
            std::cout << "consumer) finish to render !\n";
          });
        }
      }
    }
    if (event_type == SCREEN_EVENT_CLOSE) {
      std::cout << "consumer) SCREEN_EVENT_CLOSE\n";
      /* Determine that this event is due to a producer stream denying
       * permissions. */
      screen_get_event_property_iv(event, SCREEN_PROPERTY_OBJECT_TYPE,
                                   &object_type);

      if (object_type == SCREEN_OBJECT_TYPE_STREAM) {
        /* Get the handle for the producer's stream from the event. */
        screen_get_event_property_pv(event, SCREEN_PROPERTY_STREAM,
                                     (void **)&stream_p);

        if (stream_p != NULL) {
          /* Get the handle for the producer's stream ID from the event.
           * If there are multiple producers in the system, consumers can use
           * the producer's stream ID
           * as a way to verify whether the SCREEN_EVENT_CREATE event is from
           * the producer that the consumer
           * is expecting. In this example, we assume there's only one producer
           * in the system.
           */
          screen_get_stream_property_iv(stream_p, SCREEN_PROPERTY_ID,
                                        &stream_p_id);
          /* Release any buffers that have been acquired. */
          screen_release_buffer(acquired_buffer);
          /* Deregister asynchronous notifications of updates, if necessary.
             */
          screen_notify(screen_ctx, SCREEN_NOTIFY_UPDATE, stream_p, NULL);
          /* Destroy the consumer stream that's connected to this producer
             stream. */
          screen_destroy_stream(stream_c);
          /* Free up any resources that were locally allocated to track this
             stream. */
          screen_destroy_stream(stream_p);
        }
      }
    }
    // rendering??
  }
  screen_destroy_event(event);
}
