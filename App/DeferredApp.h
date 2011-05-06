#ifndef _DEFERRED_APP
#define _DEFERRED_APP

class DeferredApp
{
public:
	static DeferredApp *instance();

	void render(ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);

private:
	static DeferredApp *inst;
	DeferredApp();

	ID3D10Device *_d3d_device;
	double _time;
	double _elapsed_time;
	void *_user_context;

	void geometry_stage();
};
#endif //_DEFERRED_APP