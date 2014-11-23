/*******************************************************************************
 *  hemisampling.cpp
 *******************************************************************************
 *  Copyright (c) 2013 Alexandre Kaspar <alexandre.kaspar@a3.epfl.ch>
 *  For Advanced Computer Graphics, at the LGG / EPFL
 * 
 *        DO NOT REDISTRIBUTE
 ***********************************************/

#include <nori/vector.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief Cosine hemisphere sampling
 * 
 * \param sample a 2d uniform sample
 */
inline Vector3f squareToCosineHemisphere(const Point2f &sample) {

    float cosTheta = sqrt(sample.x());
    float sinTheta = std::sqrt(std::max((float) 0, 1-cosTheta*cosTheta));

    float sinPhi, cosPhi;
    sincosf(2.0f * M_PI * sample.y(), &sinPhi, &cosPhi);

    return Vector3f(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);

}

NORI_NAMESPACE_END
