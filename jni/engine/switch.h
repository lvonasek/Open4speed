///----------------------------------------------------------------------------------------
/**
 * \file       switch.h
 * \author     Vonasek Lubos
 * \date       2014/12/30
 * \brief      This utility switches between components. Switch depends on configuration
 *             file.
**/
///----------------------------------------------------------------------------------------

#ifndef SWITCH_H
#define SWITCH_H

#ifdef ANDROID
#define ZIP_ARCHIVE
#endif

#include <string>
#include <zip.h>
#include "engine/model.h"
#include "interfaces/input.h"
#include "interfaces/physics.h"
#include "interfaces/renderer.h"
#include "interfaces/shader.h"
#include "interfaces/texture.h"
#include "interfaces/vbo.h"

void clearMediaStorage();

/**
 * @brief addCar adds car into scene
 * @param c is instance of new car
 */
void addCar(car* c);

/**
 * @brief getCar gets car from scene
 * @param index is index of car
 * @return car instance
 */
car* getCar(int index);

/**
 * @brief getCarCount gets count of cars in scene
 * @return count of cars
 */
unsigned int getCarCount();

/**
 * @brief setShaderPath sets path of shaders
 * @param path is relative path to runable file
 */
void setShaderPath(std::string path);

/**
 * @brief getInput gets input controller
 * @return input controller
 */
input* getInput();

/**
 * @brief getModel gets model
 * @param filename is path and name of file to load
 * @return instance of model
 */
model* getModel(std::string filename);

/**
 * @brief getPhysics gets physical engine
 * @return physical engine
 */
physics* getPhysics();

/**
 * @brief getRenderer gets renderer
 * @return renderer instance
 */
renderer* getRenderer();

/**
 * @brief getShader compile shader from file
 * @param name is shader filename
 * @return instance of shader
 */
shader* getShader(std::string name);

/**
 * @brief getTexture gets texture
 * @param filename is filename of texture
 * @param alpha is amount of blending
 * @return texture instance
 */
texture* getTexture(std::string filename, float alpha);

/**
 * @brief getTexture gets texture
 * @param r is amount of red from 0 to 1
 * @param g is amount of green from 0 to 1
 * @param b is amount of blue from 0 to 1
 * @param alpha is amount of blending
 * @return texture instance
 */
texture* getTexture(float r, float g, float b, float alpha);

/**
 * @brief glvbo creates VBO from data
 * @param size is amount of vertices
 * @param vertices is vertices array
 * @param normals is normals array
 * @param coords is texture coords array
 * @param tnormals is triangle normals array
 */
vbo* getVBO(int size, float* vertices, float* normals, float* coords, float* tnormals, bool dynamic);


/**
 * @brief getZip gets APK archive object
 * @param path is "" to get last APK, there is path when APK opening is needed
 * @return zip instance
 */
zip* getZip(std::string path);

#endif // SWITCH_H