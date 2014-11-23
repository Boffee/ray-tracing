/*******************************************************************************
 *  light_integrator.cpp
 *******************************************************************************
 *  Copyright (c) 2013 Alexandre Kaspar <alexandre.kaspar@a3.epfl.ch>
 *  For Advanced Computer Graphics, at the LGG / EPFL
 * 
 *        DO NOT REDISTRIBUTE
 ***********************************************/

#include <nori/acg.h>
#include <nori/bsdf.h>
#include <nori/common.h>
#include <nori/integrator.h>
#include <nori/luminaire.h>
#include <nori/sampler.h>
#include <nori/scene.h>
#include <vector>

NORI_NAMESPACE_BEGIN

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// put your group number here!
#define GROUP_NUMBER 14
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

GROUP_NAMESPACE_BEGIN()

/**
 * \brief Simple local illumination integrator
 * using light area sampling
 */
class LightIntegrator : public Integrator {
public:

        LightIntegrator(const PropertyList &propList) {
                Q_UNUSED(propList);
        }

        /// Return the mesh corresponding to a given luminaire
        inline const Mesh *getMesh(const Luminaire *lum) const {
                const Mesh *mesh = dynamic_cast<const Mesh *> (lum->getParent());
                if (!mesh) throw NoriException("Unhandled type of luminaire!");
                return mesh;
        }

        /**
         * \brief Directly sample the lights, providing a sample weighted by 1/pdf
         * where pdf is the probability of sampling that given sample
         * 
         * \param scene
         * the scene to work with
         * 
         * \param lRec
         * the luminaire information storage
         * 
         * \param _sample
         * the 2d uniform sample
         * 
         * \return the sampled light radiance including its geometric, visibility and pdf weights
         */
        inline Color3f sampleLights(const Scene *scene, LuminaireQueryRecord &lRec, const Point2f &_sample) const {
                Point2f sample(_sample);
                const std::vector<Luminaire *> &luminaires = scene->getLuminaires();

                if (luminaires.size() == 0)
                        throw NoriException("LightIntegrator::sampleLights(): No luminaires were defined!");

                // TODO Implement the following steps
                // and take care of using the good G, V terms to work with the Li method below

                // 1. Choose one luminaire at random
                int index = std::min((int) (luminaires.size()*sample.x()), (int) luminaires.size()-1);
                lRec.luminaire = luminaires[index];

                // 2. Sample the position on the luminaire mesh
                // using Mesh::samplePosition(const Point2d &sample, Point3f &p, Normal3f &n)
                this->getMesh(lRec.luminaire)->samplePosition(sample,lRec.p,lRec.n);

                // 3. Compute geometry term G and visibility term on the luminaire's side (no cos(w) of the mesh side)
                // as well as the pdf of that point being found
                // use Mesh::pdf to get the probability of choosing the point in Mesh::samplePosition
                LuminaireQueryRecord nlRec = LuminaireQueryRecord(lRec.luminaire,lRec.ref,lRec.p,lRec.n);
                lRec = nlRec;
                float cosTheta = (-lRec.d).dot(lRec.n);

                if (cosTheta < 0) {
                    return Color3f(0.0f);
                }

                if (scene->rayIntersect(Ray3f(lRec.ref, lRec.d, Epsilon, lRec.dist * (1-1e-4f)))) {
                    return Color3f(0.0f);
                }

                // 4. Return radiance emitted from luminaire multiplied by the appropriate terms G, V ...

                if (this->getMesh(lRec.luminaire)->pdf() != 0) {
                    float geom = cosTheta/pow(lRec.dist,2);

                    return Color3f(geom * lRec.luminaire->eval(lRec));
                }
                else return Color3f(0.0f);
        }



        /**
         * \brief Simple local illumination integration:
         * We cast a ray from the camera, intersects it with the first element
         * in the scene it goes through and from there we directly sample the
         * light's in the scene to compute direct lighting.
         */
        Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray_) const {
                Ray3f ray(ray_);

                /* Find the surface that is visible in the requested direction */
                Intersection its;
                if (!scene->rayIntersect(ray, its))
                        return Color3f(0.0f);

                const Mesh *mesh = its.mesh;
                const BSDF *bsdf = mesh->getBSDF();

                /* If we hit a luminaire, use its related color information */
                if (mesh->isLuminaire()) {
                        const Luminaire *luminaire = its.mesh->getLuminaire();
                        LuminaireQueryRecord lRec(luminaire, ray.o, its.p, its.shFrame.n);
                        return luminaire->eval(lRec);
                }

                /// TODO implement direct lighting using light sampling using
                //      sampleLights(const Scene *, LuminaireQueryRecord &, const Point2f &)
                // which you also have to implement
                LuminaireQueryRecord lRec(its.p);
                const Color3f geom = sampleLights(scene, lRec, sampler->next2D());
                const Vector3f wo = its.shFrame.toLocal(lRec.d);
                const Vector3f wi = its.toLocal(-ray.d);
                BSDFQueryRecord bRec(wi, wo, ESolidAngle);
                Color3f f_r = bsdf->eval(bRec);

                return f_r*fabs(Frame::cosTheta(wo))*geom;
        }

        QString toString() const {
                return QString("LightIntegrator[]");
        }
};

GROUP_NAMESPACE_END

NORI_REGISTER_GROUP_CLASS(LightIntegrator, "light");
NORI_NAMESPACE_END
