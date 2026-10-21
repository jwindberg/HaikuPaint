#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>


class BMenuBar;
class CanvasView;


class MainWindow : public BWindow {
public:
								MainWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			BMenuBar*			_BuildMenuBar();

			CanvasView*			fCanvasView;
};


#endif	// MAIN_WINDOW_H
