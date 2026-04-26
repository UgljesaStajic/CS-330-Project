///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection before any textures are loaded
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "";
		m_textureIDs[i].ID = 0;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	DestroyGLTextures();
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	m_objectMaterials.clear();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			stbi_image_free(image);
			glBindTexture(GL_TEXTURE_2D, 0);
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
		m_textureIDs[i].ID = 0;
		m_textureIDs[i].tag = "";
	}

	m_loadedTextures = 0;
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(bFound);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load the textures used by the table, mug, plate, and pancake stack.
	// The pancake side texture is tiled
	CreateGLTexture("../../Utilities/textures/table_wood.jpg", "tableTexture");
	CreateGLTexture("../../Utilities/textures/plate_ceramic.jpg", "ceramicTexture");
	CreateGLTexture("../../Utilities/textures/plate_rim.jpg", "plateRimTexture");
	CreateGLTexture("../../Utilities/textures/pancake_top.jpg", "pancakeTopTexture");
	CreateGLTexture("../../Utilities/textures/pancake_side.jpg", "pancakeSideTexture");
	CreateGLTexture("../../Utilities/textures/coffee_top.jpg", "coffeeTexture");
	CreateGLTexture("../../Utilities/textures/stainless.jpg", "stainlessTexture");
	CreateGLTexture("../../Utilities/textures/stainless_end.jpg", "stainlessCapTexture");
	CreateGLTexture("../../Utilities/textures/aluminium.jpg", "aluminiumTexture");
	CreateGLTexture("../../Utilities/textures/napkin.jpg", "napkinTexture");

	// Add background window
	CreateGLTexture("../../Utilities/textures/background.jpg", "backgroundTexture");

	BindGLTextures();

	// Define materials
	m_objectMaterials.clear();

	OBJECT_MATERIAL tableWood;
	tableWood.ambientStrength = 0.08f;
	tableWood.ambientColor = glm::vec3(0.34f, 0.28f, 0.22f);
	tableWood.diffuseColor = glm::vec3(0.72f, 0.60f, 0.46f);
	tableWood.specularColor = glm::vec3(0.28f, 0.24f, 0.20f);
	tableWood.shininess = 12.0f;
	tableWood.tag = "tableWood";
	m_objectMaterials.push_back(tableWood);

	OBJECT_MATERIAL ceramic;
	ceramic.ambientStrength = 0.06f;
	ceramic.ambientColor = glm::vec3(0.66f, 0.65f, 0.62f);
	ceramic.diffuseColor = glm::vec3(0.82f, 0.81f, 0.78f);
	ceramic.specularColor = glm::vec3(0.34f, 0.34f, 0.32f);
	ceramic.shininess = 16.0f;
	ceramic.tag = "ceramic";
	m_objectMaterials.push_back(ceramic);

	OBJECT_MATERIAL pancake;
	pancake.ambientStrength = 0.05f;
	pancake.ambientColor = glm::vec3(0.52f, 0.42f, 0.25f);
	pancake.diffuseColor = glm::vec3(0.68f, 0.56f, 0.32f);
	pancake.specularColor = glm::vec3(0.05f, 0.05f, 0.04f);
	pancake.shininess = 4.0f;
	pancake.tag = "pancake";
	m_objectMaterials.push_back(pancake);

	OBJECT_MATERIAL coffee;
	coffee.ambientStrength = 0.04f;
	coffee.ambientColor = glm::vec3(0.16f, 0.10f, 0.06f);
	coffee.diffuseColor = glm::vec3(0.22f, 0.14f, 0.08f);
	coffee.specularColor = glm::vec3(0.03f, 0.03f, 0.03f);
	coffee.shininess = 3.0f;
	coffee.tag = "coffee";
	m_objectMaterials.push_back(coffee);

	OBJECT_MATERIAL stainless;
	stainless.ambientStrength = 0.10f;
	stainless.ambientColor = glm::vec3(0.52f, 0.52f, 0.53f);
	stainless.diffuseColor = glm::vec3(0.76f, 0.76f, 0.78f);
	stainless.specularColor = glm::vec3(0.92f, 0.92f, 0.94f);
	stainless.shininess = 28.0f;
	stainless.tag = "stainless";
	m_objectMaterials.push_back(stainless);

	OBJECT_MATERIAL glass;
	glass.ambientStrength = 0.03f;
	glass.ambientColor = glm::vec3(0.80f, 0.84f, 0.86f);
	glass.diffuseColor = glm::vec3(0.86f, 0.90f, 0.92f);
	glass.specularColor = glm::vec3(0.98f, 0.98f, 0.98f);
	glass.shininess = 18.0f;
	glass.tag = "glass";
	m_objectMaterials.push_back(glass);

	OBJECT_MATERIAL napkinPaper;
	napkinPaper.ambientStrength = 0.05f;
	napkinPaper.ambientColor = glm::vec3(0.90f, 0.89f, 0.86f);
	napkinPaper.diffuseColor = glm::vec3(0.95f, 0.94f, 0.91f);
	napkinPaper.specularColor = glm::vec3(0.10f, 0.10f, 0.10f);
	napkinPaper.shininess = 4.0f;
	napkinPaper.tag = "napkinPaper";
	m_objectMaterials.push_back(napkinPaper);

	OBJECT_MATERIAL saltCrystal;
	saltCrystal.ambientStrength = 0.04f;
	saltCrystal.ambientColor = glm::vec3(0.88f, 0.88f, 0.88f);
	saltCrystal.diffuseColor = glm::vec3(0.95f, 0.95f, 0.95f);
	saltCrystal.specularColor = glm::vec3(0.18f, 0.18f, 0.18f);
	saltCrystal.shininess = 5.0f;
	saltCrystal.tag = "saltCrystal";
	m_objectMaterials.push_back(saltCrystal);

	OBJECT_MATERIAL darkBase;
	darkBase.ambientStrength = 0.06f;
	darkBase.ambientColor = glm::vec3(0.10f, 0.10f, 0.11f);
	darkBase.diffuseColor = glm::vec3(0.16f, 0.16f, 0.17f);
	darkBase.specularColor = glm::vec3(0.18f, 0.18f, 0.20f);
	darkBase.shininess = 10.0f;
	darkBase.tag = "darkBase";
	m_objectMaterials.push_back(darkBase);

	// Enable lighting
	m_pShaderManager->setIntValue(g_UseLightingName, true);

	// Light 0: main light from the window side
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(-3.0f, 5.8f, -5.5f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.08f, 0.08f, 0.08f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.15f);

	// Light 1: warm colored fill light from front-right
    m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(6.0f, 3.2f, 5.5f));
    m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.05f, 0.04f, 0.03f));
    m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(1.0f, 0.88f, 0.72f));
    m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(1.0f, 0.92f, 0.80f));
    m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
    m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.08f);

	// Light 2: gentle overhead lift
	m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(1.2f, 6.8f, 1.2f));
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.03f, 0.03f, 0.03f));
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 14.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.06f);

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Background / window ****************************************/
	m_pShaderManager->setIntValue(g_UseLightingName, false);

	scaleXYZ = glm::vec3(24.0f, 1.0f, 12.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.8f, 6.0f, -8.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("backgroundTexture");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawPlaneMesh();

	m_pShaderManager->setIntValue(g_UseLightingName, true);

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                        ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Table texture
	SetShaderTexture("tableTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("tableWood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/*** Mug lower body **********************************************/
	scaleXYZ = glm::vec3(0.90f, 1.35f, 0.90f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.52f, 0.0f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("ceramicTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("ceramic");

	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/*** Mug upper body **********************************************/
	scaleXYZ = glm::vec3(0.92f, 0.78f, 0.92f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.52f, 1.35f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("ceramicTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("ceramic");

	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/*** Rim *********************************************************/
	scaleXYZ = glm::vec3(0.80f, 0.80f, 0.28f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.52f, 2.14f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("ceramicTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("ceramic");

	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	/*** Coffee ******************************************************/
	scaleXYZ = glm::vec3(0.78f, 0.018f, 0.64f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.52f, 2.12f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("coffeeTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("coffee");

	m_basicMeshes->DrawCylinderMesh(true, false, false);
	/****************************************************************/

	/*** Mug handle **************************************************/
	scaleXYZ = glm::vec3(0.52f, 0.60f, 0.52f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 8.0f;
	positionXYZ = glm::vec3(5.6f, 1.0f, 1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("ceramicTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("ceramic");

	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	/*** Plate body **************************************************/
	scaleXYZ = glm::vec3(2.05f, 0.12f, 2.05f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.0f, 0.0f, 0.10f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("ceramicTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("ceramic");

	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/*** Plate rim ***************************************************/
	scaleXYZ = glm::vec3(1.78f, 1.78f, 0.08f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.0f, 0.13f, 0.10f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("plateRimTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("ceramic");

	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	/*** Pancake 1 top and bottom ************************************/
	scaleXYZ = glm::vec3(1.42f, 0.14f, 1.42f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.10f, 0.12f, 0.15f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("pancakeTopTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("pancake");

	m_basicMeshes->DrawCylinderMesh(true, true, false);

	SetShaderTexture("pancakeSideTexture");
	SetTextureUVScale(2.5f, 1.0f);
	SetShaderMaterial("pancake");

	m_basicMeshes->DrawCylinderMesh(false, false, true);
	/****************************************************************/

	/*** Pancake 2 top and bottom ************************************/
	scaleXYZ = glm::vec3(1.38f, 0.14f, 1.38f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.0f, 0.24f, 0.10f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("pancakeTopTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("pancake");

	m_basicMeshes->DrawCylinderMesh(true, true, false);

	SetShaderTexture("pancakeSideTexture");
	SetTextureUVScale(2.5f, 1.0f);
	SetShaderMaterial("pancake");

	m_basicMeshes->DrawCylinderMesh(false, false, true);
	/****************************************************************/

	/*** Pancake 3 top and bottom ************************************/
	scaleXYZ = glm::vec3(1.34f, 0.14f, 1.34f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.90f, 0.36f, 0.06f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("pancakeTopTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("pancake");

	m_basicMeshes->DrawCylinderMesh(true, true, false);

	SetShaderTexture("pancakeSideTexture");
	SetTextureUVScale(2.5f, 1.0f);
	SetShaderMaterial("pancake");

	m_basicMeshes->DrawCylinderMesh(false, false, true);
	/****************************************************************/

	/*** Salt shaker: salt interior *********************************/
	scaleXYZ = glm::vec3(0.60f, 1.18f, 0.60f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.18f, 0.05f, -2.52f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.95f, 0.95f, 0.94f, 1.0f);
	SetShaderMaterial("saltCrystal");

	m_basicMeshes->DrawTaperedCylinderMesh(true, true, true);
	/****************************************************************/

	/*** Salt shaker: cap ring **************************************/
	scaleXYZ = glm::vec3(0.43f, 0.08f, 0.43f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.18f, 1.68f, -2.52f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("stainlessTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stainless");

	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/*** Salt shaker: cap dome **************************************/
	scaleXYZ = glm::vec3(0.45f, 0.23f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.18f, 1.80f, -2.52f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("stainlessCapTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stainless");

	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	/*** Napkin holder: bottom metal plate **************************/
	scaleXYZ = glm::vec3(2.6f, 0.06f, 0.78f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(6.66f, 0.04f, -0.84f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("aluminiumTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stainless");

	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/*** Napkin holder: left side metal plate ***********************/
	scaleXYZ = glm::vec3(2.6f, 2.12f, 0.06f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(6.97f, 1.13f, -1.02f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("aluminiumTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stainless");

	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/*** Napkin holder: right side metal plate **********************/
	scaleXYZ = glm::vec3(2.6f, 2.12f, 0.06f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(6.35f, 1.13f, -0.66f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("aluminiumTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("stainless");

	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/*** Napkins: stacked sheets ************************************/
	scaleXYZ = glm::vec3(2.6f, 2.90f, 0.028f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = -1.4f;
	positionXYZ = glm::vec3(6.75f, 1.52f, -0.89f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("napkinTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("napkinPaper");

	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(2.6f, 2.80f, 0.028f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = -0.9f;
	positionXYZ = glm::vec3(6.71f, 1.47f, -0.87f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("napkinTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("napkinPaper");

	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(2.6f, 2.70f, 0.028f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = -0.4f;
	positionXYZ = glm::vec3(6.68f, 1.42f, -0.85f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("napkinTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("napkinPaper");

	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(2.6f, 2.60f, 0.028f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(6.64f, 1.37f, -0.83f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("napkinTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("napkinPaper");

	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(2.6f, 2.50f, 0.028f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = 0.4f;
	positionXYZ = glm::vec3(6.61f, 1.32f, -0.81f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("napkinTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("napkinPaper");

	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(2.6f, 2.50f, 0.028f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = 0.9f;
	positionXYZ = glm::vec3(6.57f, 1.32f, -0.79f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("napkinTexture");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("napkinPaper");

	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/*** Salt shaker: glass body ************************************/
	scaleXYZ = glm::vec3(0.70f, 1.70f, 0.70f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.18f, 0.0f, -2.52f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.86f, 0.90f, 0.92f, 0.32f);
	SetShaderMaterial("glass");

	m_basicMeshes->DrawTaperedCylinderMesh(true, true, true);
	/****************************************************************/
}