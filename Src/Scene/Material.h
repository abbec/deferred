#ifndef _MATERIAL_H
#define _MATERIAL_H

namespace Deferred
{
	class Material
	{
	public:
		Material(){}

	private:
		float _specular_power;
		float _specular_intensity;
	};
}

#endif