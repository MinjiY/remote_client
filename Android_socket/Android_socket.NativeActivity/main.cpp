/*
* Copyright (C) 2010 The Android Open Source Project
*
* Apache ���̼��� 2.0 ����(���� "���̼���")�� ���� ���̼����� �ο��˴ϴ�.
* ���̼����� �ؼ����� ������ �� ������ ����� �� �����ϴ�.
* ���̼����� �纻��
*
*      http://www.apache.org/licenses/LICENSE-2.0���� ���� �� �ֽ��ϴ�.
*
* ���� ������ ������ ���� �ʿ��ϰų� �������� �������� �ʴ� �̻�
* ���̼����� ���� �����Ǵ� ����Ʈ����� "�ִ� �״��",
* ����� �Ǵ� �������̵� ��� ������ �����̳� ���� ���� �����˴ϴ�.
* ���̼����� ���� Ư�� ����� ���� �� ���ѿ� ���� ������
* ���̼����� �����ϼ���.
*
*/

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

/**
* ����� ���� �������Դϴ�.
*/
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
};

/**
* �ۿ� ���� ���� �����Դϴ�.
*/
struct engine {
	struct android_app* app;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	const ASensor* gyroscope_sensor;
	const ASensor* magnet_sensor;
	ASensorEventQueue* p_sensor_queue;

	int animating;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;
	struct saved_state state;
};

/**
* ���� ���÷��̿� ���� EGL ���ؽ�Ʈ�� �ʱ�ȭ�մϴ�.
*/
static int engine_init_display(struct engine* engine) {
	// OpenGL ES �� EGL �ʱ�ȭ

	/*
	* ���⿡�� ���ϴ� ������ Ư���� �����մϴ�.
	* �Ʒ����� ȭ�� â�� ȣȯ�Ǵ� 
	* ���� ���� ��Ҵ� �ּ� 8��Ʈ�� ���Ե� EGLConfig�� �����߽��ϴ�.
	*/
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* ���⿡�� ���� ���α׷��� ���ϴ� ������ �����մϴ�. �� ���ÿ���
	* ���ؿ� ��ġ�ϴ� ù ��° EGLConfig�� �����ϴ�
	* ���� ����ȭ�� ���� ������ ���ԵǾ� �ֽ��ϴ�.*/
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID�� EGLConfig�� Ư������
	* ANativeWindow_setBuffersGeometry()���� ���εǵ��� �����մϴ�.
	* EGLConfig�� �����ϸ� EGL_NATIVE_VISUAL_ID�� ����Ͽ� 
	* ANativeWindow ���۰� ��ġ�ϵ��� �����ϰ� �ٽ� ������ �� �ֽ��ϴ�. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	engine->state.angle = 0;

	// GL ���¸� �ʱ�ȭ�մϴ�.
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);

	return 0;
}


/**
* ���÷����� ���� �������Դϴ�.
*/

#define SetRectF(p,sx,sy,ex,ey)  {p[0]=sx; p[1]=ey; p[2]=ex; p[3]=ey; p[4]=ex; p[5]=sy; p[6]=sx; p[7]=sy;}
#define INNER_X_MARGIN  0.02f
#define INNER_Y_MARGIN  0.01f


static void engine_draw_frame(struct engine* engine) {
	if (engine->display == NULL) {
		// ���÷��̰� �����ϴ�.
		return;
	}

	// ȭ���� �������� ä��ϴ�.
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	int y, x;
	GLfloat pos[8];

	glEnableClientState(GL_VERTEX_ARRAY);
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 2; x++) {
			glColor4f(0.0, 0.3, 0.3, 1);
			SetRectF(pos, -1 + x * 1 + INNER_X_MARGIN, 1 - y*0.25 - INNER_Y_MARGIN, -1 + (x + 1) * 1 - INNER_X_MARGIN, 1 - (y + 1)*0.25 + INNER_Y_MARGIN);
			glVertexPointer(2, GL_FLOAT, 0, pos);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glColor4f(0.0, 0.05, 0.15, 1);
			SetRectF(pos, -1 + x * 1 + INNER_X_MARGIN + INNER_X_MARGIN, 1 - y*0.25 - INNER_Y_MARGIN - INNER_Y_MARGIN, -1 + (x + 1) * 1 - INNER_X_MARGIN - INNER_X_MARGIN, 1 - (y + 1)*0.25 + INNER_Y_MARGIN + INNER_Y_MARGIN);
			glVertexPointer(2, GL_FLOAT, 0, pos);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		}
	}
	glDisableClientState(GL_VERTEX_ARRAY);

	eglSwapBuffers(engine->display, engine->surface);
}

/**
* ���� ���÷��̿� ����� EGL ���ؽ�Ʈ�� �����մϴ�.
*/
static void engine_term_display(struct engine* engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->animating = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

int gh_socket;

void SendFrameData(int ah_socket,
	unsigned char a_msg_id,
	const char *ap_data,
	unsigned short int a_data_size)
{
	int send_data_size = a_data_size + 4;
	char *p_send_data = new char[send_data_size];

	*p_send_data = 27;
	*(p_send_data + 1) = a_msg_id;
	*(unsigned short int *)(p_send_data + 2) = a_data_size;
	memcpy(p_send_data + 4, ap_data, a_data_size);
	send(ah_socket, p_send_data, send_data_size, 0);
	delete[] p_send_data;
}

/**
* ���� �Է� �̺�Ʈ�� ó���մϴ�.
*/
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {          // ��ġ�� ���� ó���� �ϰڴ�. (Ű�� ���� ó���� ����)
		if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_DOWN) {        // ��ġ �ٿ ���� ó�� ( ��,���꿡 ���� ó���� ����)
			engine->state.x = AMotionEvent_getX(event, 0);
			engine->state.y = AMotionEvent_getY(event, 0);

			int step_x = engine->width / 2;
			int step_y = engine->height / 8;

			int x = engine->state.x / step_x;
			int y = engine->state.y / step_y;

			int index = y * 2 + x;

			char temp[16];
			int length = sprintf(temp, "%d", index);
			SendFrameData(gh_socket, 1, temp, length + 1);
			SendFrameData(gh_socket, 2, (char *)&index, sizeof(int));
			return 1;
		}
	}
	return 0;
}

void StartSensor(engine* ap_engine)
{
	if (ap_engine->accelerometerSensor != NULL) {
		ASensorEventQueue_enableSensor(ap_engine->p_sensor_queue,
			ap_engine->accelerometerSensor);
		// �ʴ� 60���� �̺�Ʈ�� ������ �� �ֽ��ϴ�.
		ASensorEventQueue_setEventRate(ap_engine->p_sensor_queue,
			ap_engine->accelerometerSensor, (1000L / 60) * 1000);
	}
	if (ap_engine->gyroscope_sensor != NULL) {
		ASensorEventQueue_enableSensor(ap_engine->p_sensor_queue,
			ap_engine->gyroscope_sensor);
		// �ʴ� 60���� �̺�Ʈ�� ������ �� �ֽ��ϴ�.
		ASensorEventQueue_setEventRate(ap_engine->p_sensor_queue,
			ap_engine->gyroscope_sensor, (1000L / 60) * 1000);
	}
	if (ap_engine->magnet_sensor != NULL) {
		ASensorEventQueue_enableSensor(ap_engine->p_sensor_queue,
			ap_engine->magnet_sensor);
		// �ʴ� 60���� �̺�Ʈ�� ������ �� �ֽ��ϴ�.
		ASensorEventQueue_setEventRate(ap_engine->p_sensor_queue,
			ap_engine->magnet_sensor, (1000L / 60) * 1000);
	}
}

void StopSensor(engine* ap_engine)
{
	if (ap_engine->accelerometerSensor != NULL) {
		ASensorEventQueue_disableSensor(ap_engine->p_sensor_queue, ap_engine->accelerometerSensor);
	}
	if (ap_engine->gyroscope_sensor != NULL) {
		ASensorEventQueue_disableSensor(ap_engine->p_sensor_queue, ap_engine->gyroscope_sensor);
	}
	if (ap_engine->magnet_sensor != NULL) {
		ASensorEventQueue_disableSensor(ap_engine->p_sensor_queue, ap_engine->magnet_sensor);
	}
}

/**
* ���� �� ����� ó���մϴ�.
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// �ý��ۿ��� ���� ���¸� �����ϵ��� ��û�߽��ϴ�. �����ϼ���.
		engine->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// â�� ǥ�õǾ� �غ� ���ƽ��ϴ�.
		if (engine->app->window != NULL) {
			engine_init_display(engine);
			engine_draw_frame(engine);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// â�� ����ų� �ݾ� �����մϴ�.
		engine_term_display(engine);
		break;
	case APP_CMD_GAINED_FOCUS:
		// �ۿ� ��Ŀ���� ������ ���ӵ��� ����͸��� �����մϴ�.
		StartSensor(engine);
		engine->animating = 1;
		break;
	case APP_CMD_LOST_FOCUS:
		// �ۿ��� ��Ŀ���� ������� ���ӵ��� ����͸��� �����˴ϴ�.
		// ������� �ʴ� ���� ���͸��� �����ϱ� ���� ��ġ�Դϴ�.
		StopSensor(engine);
		// �ִϸ��̼ǵ� �����˴ϴ�.
		engine->animating = 0;
		engine_draw_frame(engine);
		break;
	}
}

/**
* android_native_app_glue�� ����ϴ� ����Ƽ�� ���� ���α׷���
* �� �������Դϴ�. ��ü �����忡�� ����Ǹ�, �Է� �̺�Ʈ��
* �ް� �ٸ� �۾��� �����ϴ� ��ü �̺�Ʈ ������ �����մϴ�.
*/




void android_main(struct android_app* state) {
	struct engine engine;
	TipsGyroscopeSensor gyro_sensor_manager;

	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;

	// ���ӵ��� ����͸��� �غ��մϴ�.
	engine.sensorManager = ASensorManager_getInstance();
	engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager, ASENSOR_TYPE_ACCELEROMETER);
	engine.gyroscope_sensor = ASensorManager_getDefaultSensor(engine.sensorManager, ASENSOR_TYPE_GYROSCOPE);
	engine.magnet_sensor = ASensorManager_getDefaultSensor(engine.sensorManager, ASENSOR_TYPE_MAGNETIC_FIELD);
	engine.p_sensor_queue = ASensorManager_createEventQueue(engine.sensorManager, state->looper, LOOPER_ID_USER, NULL, NULL);

	if (state->savedState != NULL) {
		// ������ ����� ���·� ���۵Ǹ�, �� �������� �����˴ϴ�.
		engine.state = *(struct saved_state*)state->savedState;
	}



	gh_socket = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in srv_addr;
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = inet_addr("172.30.1.33");
	srv_addr.sin_port = htons(25790);

	if (connect(gh_socket, (sockaddr *)&srv_addr, sizeof(srv_addr)) == 0) {
		SendFrameData(gh_socket, 1, "Hello~", 7);
	}

	engine.animating = 1;

	//������ �۾��� ����ϸ鼭 ������ �����մϴ�.

	while (1) {
		// ���� ���� ��� �̺�Ʈ�� �н��ϴ�.
		int ident;
		int events;
		struct android_poll_source* source;
		float temp_data[3];
		// �ִϸ��̼��� �������� ������ �̺�Ʈ ��⸦ ���������� �����մϴ�.
		// �ִϸ��̼��� �����ϸ� ��� �̺�Ʈ�� ���� ������ ������ ������ ����
		// ����ؼ� �ִϸ��̼��� ���� �������� �׸��ϴ�.
		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,	(void**)&source)) >= 0) {

			// �� �̺�Ʈ�� ó���մϴ�.
			if (source != NULL) {source->process(state, source);}

			// ������ �����Ͱ� ������ �ٷ� ó���˴ϴ�.
			if (ident == LOOPER_ID_USER) {
					ASensorEvent event;
					while (ASensorEventQueue_getEvents(engine.p_sensor_queue, &event, 1) > 0) {
						if (event.type == ASENSOR_TYPE_ACCELEROMETER) {
							gyro_sensor_manager.SetAccelerometerData(event.data);
						}
						else if (event.type == ASENSOR_TYPE_MAGNETIC_FIELD) {
							gyro_sensor_manager.SetMagneticFieldData(event.data);
						}
						else if (event.type == ASENSOR_TYPE_GYROSCOPE) {
							if (gyro_sensor_manager.SetGyroscopeData(event.data, event.timestamp, temp_data) == 1) {
								SendFrameData(gh_socket, 22, (const char*)temp_data, sizeof(float) * 3);
							}
						}
					}
			}

			// ���� ������ Ȯ���մϴ�.
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}

		if (engine.animating) {
			// �̺�Ʈ�� ������ �� ���� �ִϸ��̼� �������� �׸��ϴ�.
			/* engine.state.angle += .01f;
			if (engine.state.angle > 1) {
				engine.state.angle = 0;
			}
			*/
			// �׸���� ȭ�� ������Ʈ �ӵ��� ������ �����Ƿ�
			// ���⿡���� Ÿ�̹��� ����� �ʿ䰡 �����ϴ�.
			engine_draw_frame(&engine);
		}
	}
}
