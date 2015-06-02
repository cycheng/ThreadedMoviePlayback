Content:
========
(1) How to run
(2) Multithreading Design
(3) Effect Design
(4) About QThread
(5) Test

//-----------------------------------------------------------------------------
// How to run
//-----------------------------------------------------------------------------
* Option1: build from source
	Open Project/ThreadedMoviePlayback.sln, then build

* Opiton2:
	Directly run ThreadedMoviePlayback.exe, but you should have those required
	dll in the same directory

//-----------------------------------------------------------------------------
// Multithreading Design
//-----------------------------------------------------------------------------

* Triple and Double Buffer Pseudo Code

	Triple buffer case:
	===================
		3 buffers: working, workingCopy, stable
		1 flag: workingCopyEmpty

		worker thread:
			write to working
			enter critical section
				wait until workingCopy is empty (workingCopyEmpty == true)
				swap working and workingCopy

		render thread:
			enter critical section
				if workingCopy full, swap workingCopy and stable (workingCopyEmpty == false)
				else, use old stable
					=> So we should initialise workingCopy or stable
			read from stable

	Double buffer case:
	===================
		2 buffers: working, stable
		1 flag: workingFull

		worker thread:
			write to working
			enter critical section
				set workingFull to true
				wait until workingFull become false

		render thread:
			enter critical section
				if working full, swap working and stable (workingFull == true)
				else, use old stable
					=> So we should initialise stable
			read from stable

* My v1.0-design [0006-Thread-implement-v1.0-workable-version-pass-window-r.patch]

	Class Design:
	=============
		class CTripleBuffer
		{
		public:
			unsigned char* GetWorkingBuffer();
			unsigned char* GetStableBuffer();
			void Wakeup();
		private:
			QMutex m_mutex;
			QWaitCondition m_emptySignal;

			buffers and flag;
		};

		class CWorker
		{
		public:
			void run() override;
			void Pause();
			void Stop();
			void Resume();
		private:
			QMutex m_mutex;
			QWaitCondition m_runSignal;
			QWaitCondition m_pauseSignal;

			flow control flags;
		};

	Flow:
	=====
		worker thread (CWorker) will do some thread flow management (Pause, ..),
		then call GetWorkingBuffer() to get an empty working buffer, may block.

		render thread call GetStableBuffer() to get an updated stable buffer.

	Drawback:
	=========
		It becomes complex for thread flow management, and CTripleBuffer have to
		export strange API, e.g. Wakeup(), just because of CTripleBuffer own
		concurrency related code.

* My v2.0-design [0007-Thread-implement-v2.0-Re-design-CTripleBuffer.-Separ.patch]

	Class Design:
	=============
	class CTripleBuffer
	{
		public:
			unsigned char* GetWorkingBuffer();
			unsigned char* GetStableBuffer();
		private:
			buffers and flag;
	};

	class CWorker
	{
	public:
		const CBuffer* GetUpdatedBufferAndSignalWorker();
		.. as before;
	};

	Flow:
	=====
		Now, CTripleBuffer just a buffer without concurrency code. All
		concurrency related code are moved to CWorker.

		render thread call GetUpdatedBufferAndSignalWorker() to get an updated
		stable buffer (or unchanged buffer)

//-----------------------------------------------------------------------------
// Effect Design
//-----------------------------------------------------------------------------

* Blue print:
	FX_BASE ----+ (or)
				|
				+-> FX_FLUID -> FX_PAGECURL
				|
	FX_FRACTAL -+ (or)

* Brief:
	FX_BASE or FX_FRACTAL generate output to framebuffer.
	FX_FLUID generate smoke, and do alpha blending (src_alpha, 1 - src_alpha)
		on framebuffer

	FX_PAGECURL takes the framebuffer as input texture for page curling.

	In this demo, you can freely enable/disable FX_FRACTAL, FX_FLUID, FX_PAGECURL.
	When you disable FX_PAGECURL, previous FX will render to window framebuffer.

* Each effect:
	FX_BASE: movie playback
	FX_FRACTAL: movie playback blending with fractal effect
	FX_FLUID: smoke fluid simulation. you can drag the circle to interact with smoke
	FX_PAGECURL: page curl effect

* Effect source:
	FX_PAGECURL:
	http://blog.rectalogic.com/webvfx/examples_2transition-shader-pagecurl_8html-example.html

	FX_FLUID:
	http://prideout.net/blog/?p=58
	by Philip Rideout

//-----------------------------------------------------------------------------
// About QThread
//-----------------------------------------------------------------------------

* Recommended QThread usage:
	https://mayaposch.wordpress.com/2011/11/01/how-to-really-truly-use-qthreads-the-full-explanation/
	http://blog.wupingxin.net/2012/05/06/qthread-the-way-of-do-it-right/

	"... when using a QThread is that it¡¦s not a thread. It¡¦s a wrapper around a
	 thread object. This wrapper provides the signals, slots and methods to
	 easily use the thread object within a Qt project ..."

	Don't: sub-class QThread and implement run()

	Recommended:
		Prepare an object (QObject) class with all your desired functionality in it.
		Bind it with QThread

* In this demo:
	I am still using the "Don't" method for simplicity.

//-----------------------------------------------------------------------------
// Test
//-----------------------------------------------------------------------------

* Buffer mode change test
	Changing the buffer mode (Triple buffer <--> Double buffer) every 500 ms.

	We should not aware the buffer mode is changing, the screen should not flash

* Resize test
	Resizing the window freely, and test on triple and double buffer modes.
	(manally test)

	Each resize should not take long time. So those time consuming visual effect
	should provide a method to early terminate effect update.

* Open new Video File test
* Default video unexist test
