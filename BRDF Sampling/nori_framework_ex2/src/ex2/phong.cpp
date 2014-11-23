/*******************************************************************************
 *  phong.cpp
 *******************************************************************************
 *  Copyright (c) 2013 Alexandre Kaspar <alexandre.kaspar@a3.epfl.ch>
 *  For Advanced Computer Graphics, at the LGG / EPFL
 * 
 *        DO NOT REDISTRIBUTE
 ***********************************************/

#include <nori/acg.h>
#include <nori/bsdf.h>
#include <nori/frame.h>
#include "hemisampling.cpp"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <nori/transform.h>


NORI_NAMESPACE_BEGIN

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// put your group number here!
#define GROUP_NUMBER 0
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

GROUP_NAMESPACE_BEGIN()

/**
 * \brief Phong BRDF model
 */
class Phong : public BSDF {
public:

    Phong(const PropertyList &propList) {
        m_Kd = propList.getColor("kd", Color3f(0.5f));
        m_Ks = propList.getColor("ks", Color3f(0.5f));
        m_exp = propList.getFloat("n", 20.0f);
        srand(time(NULL));
    }

    /// Reflection in local coordinates
    inline Vector3f reflect(const Vector3f &wi) const {
        return Vector3f(-wi.x(), -wi.y(), wi.z());
    }

    /// Evaluate the BRDF model

    Color3f eval(const BSDFQueryRecord &bRec) const {
        /* This is a smooth BRDF -- return zero if the measure
           is wrong, or when queried for illumination on the backside */
        if (bRec.measure != ESolidAngle
                || Frame::cosTheta(bRec.wi) <= 0
                || Frame::cosTheta(bRec.wo) <= 0)
            return Color3f(0.0f);

        // Based on http://www.cs.virginia.edu/~jdl/importance.doc‎

        // TODO evaluate the phong brdf for the pair of given incoming
        //      and outgoing directions bRec.wi & bRec.wo
        Vector3f L = bRec.wo;
        Vector3f V = bRec.wi;
        Vector3f R = Vector3f(0.0f,0.0f,2.0f*Frame::cosTheta(L))-L;
        float cosAlpha = R.dot(V);
        float brdf;

        // need to include <math.h> for M_PI
        float kd = m_Kd.getLuminance();
        float ks = m_Ks.getLuminance();
        float SamplingWeight = ks / (ks + kd);
        brdf = ks*pow(cosAlpha,m_exp)*(m_exp+2)/(2*M_PI) + kd/M_PI;
        return Color3f(brdf);
    }

    /// Compute the density of \ref sample() wrt. solid angles

    float pdf(const BSDFQueryRecord &bRec) const {
        /* This is a smooth BRDF -- return zero if the measure
           is wrong, or when queried for illumination on the backside */
        if (bRec.measure != ESolidAngle
                || Frame::cosTheta(bRec.wi) <= 0
                || Frame::cosTheta(bRec.wo) <= 0)
            return 0.0f;

        // TODO implement the pdf of phong importance sampling

        float prob;
        Vector3f L = bRec.wo;
        Vector3f V = bRec.wi;
        Vector3f R = Vector3f(0.0f,0.0f,2.0f*Frame::cosTheta(L))-L;
        float cosAlpha = R.dot(V);

        float kd = m_Kd.getLuminance();
        float ks = m_Ks.getLuminance();
        float specSamplingWeight = ks / (ks + kd);
        prob = specSamplingWeight*pow(cosAlpha,m_exp)*(m_exp+2)/(2*M_PI) + (1-specSamplingWeight)*Frame::cosTheta(bRec.wi)/M_PI;
        return prob;
    }

    /// Draw a a sample from the BRDF model

    Color3f sample(BSDFQueryRecord &bRec, const Point2f &sample_) const {
        Point2f sample(sample_);
        if (Frame::cosTheta(bRec.wi) <= 0)
            return Color3f(0.0f);

        float r = float(rand())/RAND_MAX;
        // 1. Select diffuse or specular
        float kd = m_Kd.getLuminance();
        float ks = m_Ks.getLuminance();
        float specSamplingWeight = ks / (ks + kd);
        bool useSpecular = true;
        if (r > specSamplingWeight) {
            useSpecular = false;
        }

        bRec.measure = ESolidAngle;
        bRec.eta = 1.0f; // no change in relative index

        // TODO implement phong importance sampling
        float theta;
        float phi = 2*M_PI*sample.y();
        if (useSpecular) {
            Vector3f V = bRec.wi;
            Vector3f L = Vector3f(0.0f,0.0f,2.0f*Frame::cosTheta(V))-V;
            float thetaL = acos(L.z());
            float phiL = acos(L.x()/sin(thetaL));

            theta = acos(pow(sample.x(),1.0f/(m_exp+1)));

            float x, y, z;
            Vector3f s = Vector3f(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));

            // rotate by theta of L alone the y-axis
            x = s.x()*cos(thetaL)+s.z()*sin(thetaL);
            y = s.y();
            z = -s.x()*sin(thetaL)+s.z()*cos(thetaL);
            s = Vector3f(x,y,z);

            // rotate by phi of L alone the z-axis
            x = s.x()*cos(phiL)-s.y()*sin(phiL);
            y = s.x()*sin(phiL)+s.y()*cos(phiL);
            z = s.z();
            s = Vector3f(x,y,z);
            bRec.wo = s;
        }
        else {
            theta = acos(sqrt(sample.x()));

            bRec.wo = Vector3f(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
        }

        Color3f brdf = Color3f(eval(bRec).getLuminance()*cos(theta)/pdf(bRec));
        if (brdf.getLuminance() < 0) return Color3f(0.0f);
        return brdf;
    }

    /// Return a human-readable summary

    QString toString() const {
        return QString(
                "Phong[\n"
                "  Kd = %1\n"
                "  Ks = %2\n"
                "  n  = %3\n"
                "]").arg(m_Kd.toString()).arg(m_Ks.toString()).arg(m_exp);
    }

    Color3f getColor() const {
        return m_Kd;
    }

    EClassType getClassType() const {
        return EBSDF;
    }
private:
    Color3f m_Kd, m_Ks;
    float m_exp;
};

GROUP_NAMESPACE_END

NORI_REGISTER_GROUP_CLASS(Phong, "phong");
NORI_NAMESPACE_END
