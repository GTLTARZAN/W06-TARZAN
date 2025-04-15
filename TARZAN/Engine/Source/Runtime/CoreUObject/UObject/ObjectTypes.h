#pragma once


enum OBJECTS
{
	OBJ_StaticMesh,        // UI
	OBJ_CUBE,              // UI
	OBJ_SPHERE,            // UI
	OBJ_Cylinder,          // UI
	OBJ_Plane,             // UI
	OBJ_Cone,              // UI
	OBJ_Torus,             // UI
	OBJ_SpotLight,         // UI
    OBJ_PointLight,        // UI
    OBJ_DirectionalLight,  // UI
	OBJ_Particle,          // UI
	OBJ_Text,              // UI
    OBJ_Fog,               // UI
	OBJ_TRIANGLE,
	OBJ_CAMERA,
	OBJ_PLAYER,
	OBJ_END
};
enum ARROW_DIR
{
	AD_X,
	AD_Y,
	AD_Z,
	AD_END
};
enum ControlMode
{
	CM_TRANSLATION,
	CM_ROTATION,
	CM_SCALE,
	CM_END
};
enum CoordiMode
{
	CDM_WORLD,
	CDM_LOCAL,
	CDM_END
};
enum EPrimitiveColor
{
	RED_X,
	GREEN_Y,
	BLUE_Z,
	NONE,
	RED_X_ROT,
	GREEN_Y_ROT,
	BLUE_Z_ROT
};
