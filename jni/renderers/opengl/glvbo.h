//----------------------------------------------------------------------------------------
/**
 * \file       glvbo.h
 * \author     Vonasek Lubos
 * \date       2014/02/11
 * \brief      Vertex buffer object for OpenGL
*/
//----------------------------------------------------------------------------------------

#ifndef GLVBO_H
#define GLVBO_H

#include "stdafx.h"

class glvbo : public vbo
{
public:

    GLuint instance;         ///< VBO for current object

    /**
     * @brief glvbo creates VBO from data
     * @param size is amount of vertices
     * @param vertices is vertices array
     * @param normals is normals array
     * @param coords is texture coords array
     * @param tid is special array for lightmaps
     */
    glvbo(int size, float* vertices, float* normals, float* coords, float* tid);

    /**
     * @brief bind binds VBO
     */
    void bind();

    /**
     * @brief destroy removes all data from memory
     */
    void destroy();

    /**
     * @brief unbind unbinds VBO
     */
    void unbind();
};

#endif // GLVBO_H