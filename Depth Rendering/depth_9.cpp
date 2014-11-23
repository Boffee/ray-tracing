/*******************************************************************************
 *  depth.cpp
 *******************************************************************************
 *  Copyright (c) 2013 Alexandre Kaspar <alexandre.kaspar@a3.epfl.ch>
 *  For Advanced Computer Graphics, at the LGG / EPFL
 * 
 *        DO NOT REDISTRIBUTE
 ***********************************************/

#include <nori/acg.h>
#include <nori/integrator.h>
#include <nori/sampler.h>
#include <nori/scene.h>

NORI_NAMESPACE_BEGIN

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// put your group number here!
#define GROUP_NUMBER 0
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

GROUP_NAMESPACE_BEGIN()

/**
 * \brief Depth mapping: a simple rendering technique that 
 * displays the depth of an object.
 */
class Depth : public Integrator {
public:
	Depth(const PropertyList &propList) {
                /* Depth near and far plane distance */
                m_near = propList.getFloat("near", 1e-4);
                m_far = propList.getFloat("far", 1e2);
                /* Min intensity */
                m_Ka = propList.getFloat("ambiant", 0.1);
                m_gamma = propList.getFloat("gamma", 5.0);
	}

    Color3f Li(const Scene *scene, Sampler *, const Ray3f &ray) const {
                
		/* Find the surface that is visible in the requested direction */
//        float distance;
//        Color3f weight;
//        if (!scene->sampleDistance(ray,sampler,distance,weight)){
//            qDebug() << distance;
//            return Color3f(0.0f);
//        }
        Intersection its;
        if (!scene->rayIntersect(ray, its)) {
            return Color3f(0.0f);
        }
        qDebug() << its.t;
        float m_Kd = (m_far-its.t)/(m_far-m_near);
        return m_Ka+(1-m_Ka)*pow(m_Kd,m_gamma);
    }

	QString toString() const {
		return QString("Depth[near=%1, far=%2, Ka=%3]").arg(m_near).arg(m_far).arg(m_Ka);
	}
private:
	float m_near, m_far, m_Ka, m_gamma;
};

GROUP_NAMESPACE_END

NORI_REGISTER_GROUP_CLASS(Depth, "depth");
NORI_NAMESPACE_END
