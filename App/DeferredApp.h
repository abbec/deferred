#ifndef _DEFERRED_APP
#define _DEFERRED_APP
#include "../Scene/Scene.h"

class DeferredApp
{
public:
	static DeferredApp *instance();

	HRESULT initScene(ID3D10Device* device);
	bool initBuffers(ID3D10Device* device);
	void render(ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);

	inline Scene *getScene() { return &_scene; }

	void update(double fTime, float fElapsedTime, void* pUserContext);

	LRESULT handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static DeferredApp *inst;
	DeferredApp();

	ID3D10Device *_d3d_device;
	double _time;
	double _elapsed_time;
	void *_user_context;

	Scene _scene;

	void geometry_stage();
};
#endif //_DEFERRED_APP