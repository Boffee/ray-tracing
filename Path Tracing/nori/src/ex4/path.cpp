/*******************************************************************************
 *  path.cpp
 *******************************************************************************
 *  Copyright (c) 2013 Alexandre Kaspar <alexandre.kaspar@a3.epfl.ch>
 *  For Advanced Computer Graphics, at the LGG / EPFL
 *
 *        DO NOT REDISTRIBUTE
 ***********************************************/

#include <nori/acg.h>
#include <nori/integrator.h>
#include <nori/sampler.h>
#include <nori/luminaire.h>
#include <nori/bsdf.h>
#include <nori/medium.h>
#include <nori/phase.h>
#include <nori/scene.h>

NORI_NAMESPACE_BEGIN

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// put your group number here!
#define GROUP_NUMBER 0
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

GROUP_NAMESPACE_BEGIN()

/**
 * Simple path tracer implementation
 */
class PathTracer : public Integrator {
public:

        PathTracer(const PropertyList &) {
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

                // 1. Choose one luminaire at random
                int index = std::min((int) (luminaires.size() * sample.x()), (int) luminaires.size() - 1);
                sample.x() = luminaires.size() * sample.x() - index; // process sample to be Unif[0;1] again

                // 2. Sample the position on the luminaire mesh
                // using Mesh::samplePosition(const Point2d &sample, Point3f &p, Normal3f &n)
                lRec.luminaire = luminaires[index];
                const Mesh *mesh = getMesh(lRec.luminaire);
                mesh->samplePosition(sample, lRec.p, lRec.n);
                lRec.d = lRec.p - lRec.ref;

                // 3. Compute distance between the two points (from first mesh, to luminaire mesh)
                float dist2 = lRec.d.squaredNorm();
                lRec.dist = std::sqrt(dist2);
                lRec.d /= lRec.dist;

                // 4. Correct side of luminaire
                // /!\ if on the wrong side, then we get no contribution!
                float dp = -lRec.n.dot(lRec.d);
                lRec.pdf = dp > 0 ? mesh->pdf() * dist2 / dp : 0.0f;

                if (dp > 0) {
                        // 5. Check the visibility
                        if (scene->rayIntersect(Ray3f(lRec.ref, lRec.d, Epsilon, lRec.dist * (1 - 1e-4f))))
                                return Color3f(0.0f);
                        // 6. Geometry term on luminaire's side
                        // Visiblity + Geometric term on the luminaire's side
                        //      G(x, x', w, w') = ( cos(w) cos(w') ) / ||x - x'||^2
                        float G_lum = dp / dist2;

                        // 7. Radiance from luminaire
                        Color3f value = lRec.luminaire->getColor();

                        return value * G_lum * luminaires.size() / mesh->pdf();
                } else {
                        // wrong side of luminaire!
                        return Color3f(0.0f);
                }
        }

        Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &_ray) const {
            // use inLi(...) directly
            return inLi(scene, sampler, _ray, 0.1f, 1);
        }

        Color3f inLi(const Scene *scene, Sampler *sampler, const Ray3f &_ray, float q, int iter) const {
                Ray3f ray(_ray);
                Intersection its;

                if (!scene->rayIntersect(ray, its)) {
                    return Color3f(0.0f);
                }

                const Mesh *mesh = its.mesh;
                const BSDF *bsdf = mesh->getBSDF();

                if (mesh->isLuminaire()) {
                    // only return for 1st ray from eye because the rest the rest are calculate once already through sampleLight
                    // so if any ray after the 1st ray would be sampled twice if they hit the light by chance
                    if (iter == 1) {
                       const Luminaire *luminaire = its.mesh->getLuminaire();
                       LuminaireQueryRecord lRec(luminaire, ray.o, its.p, its.shFrame.n);
                       return luminaire->eval(lRec);
                    }
                    else return (0.0f);
                }

                // remove this because you want to keep sampling for indirect lighting if you can hit a light source form your location
                // otherwise, occluded regions would never recieve any illumination
//                if ((s.array() == 0).all()) {
//                    return Color3f(0.0f);
//                }

                LuminaireQueryRecord lRec(its.p);
                Color3f s = sampleLights(scene, lRec, sampler->next2D());

                BSDFQueryRecord bRec(its.toLocal(-ray.d), its.toLocal(lRec.d), ESolidAngle);
                Color3f t = scene->evalTransmittance(Ray3f(lRec.ref, lRec.d, 0, lRec.dist), sampler);
                Color3f c = bsdf->eval(bRec) * s * t * std::abs(Frame::cosTheta(bRec.wo));

                // add iteration tracking
                if (iter >= 2 && !russianRoullette(sampler, q)) {
                    return c;
                }

                BSDFQueryRecord bRec2(its.toLocal(-ray.d));
                Color3f fr = bsdf->sample(bRec2, sampler->next2D());

                // if fr is 0, theres no point of computing the next iteration
                if ((fr.array() == 0).all()) {
                    return c;
                }

                Ray3f nextRay = Ray3f(its.p, its.shFrame.toWorld(bRec2.wo));
                // start dividing by 1-q once the russian roulette starts after 2 iteartions
                if (iter >= 2) {
                    return c + (fr * inLi(scene, sampler, nextRay, q, iter+1))/(1-q);
                }
                else {
                    return c + fr * inLi(scene, sampler, nextRay, q, iter+1);
                }
        }

        // Do we stop?
        inline bool russianRoullette(Sampler *sampler, float q) const {
                //return true;
                return sampler->next1D() > q;
        }


        QString toString() const {
                return "PathTracer[]";
        }
};

GROUP_NAMESPACE_END

NORI_REGISTER_GROUP_CLASS(PathTracer, "path");
NORI_NAMESPACE_END
