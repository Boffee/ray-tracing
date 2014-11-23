/*
	Path tracer reference implementation (without multiple importance sampling)
    Copyright (c) 2012 by Wenzel Jakob and Steve Marschner.

	DO NOT REDISTRIBUTE
*/

#include <nori/integrator.h>
#include <nori/sampler.h>
#include <nori/luminaire.h>
#include <nori/bsdf.h>
#include <nori/medium.h>
#include <nori/phase.h>
#include <nori/scene.h>

NORI_NAMESPACE_BEGIN

/// Simple Kajiya-style path tracer
class PathTracer : public Integrator {
public:
	PathTracer(const PropertyList &) { }

	Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &_ray) const {
		Ray3f ray(_ray);
		Intersection its;
		Color3f result(0.0f), throughput(1.0f), mediumWeight;
		int depth = 0;
		bool includeEmitted = true;
		float eta = 1.0f, traveledDistance;

		while (true) {
			/* Find the surface that is visible in the requested direction (if any) */
			scene->rayIntersect(ray, its);
		
			/* Find the traveled distance within the scene's participating medium (if any) */
			if (scene->sampleDistance(Ray3f(ray, 0, its.t), sampler, traveledDistance, mediumWeight)) {
				throughput *= mediumWeight;

				const Medium *medium = scene->getMedium();
				const PhaseFunction *phase = medium->getPhaseFunction();

				/* 1. Direct illumination sampling */
				LuminaireQueryRecord lRec(ray(traveledDistance));
				Color3f direct = scene->sampleDirect(lRec, sampler->next2D());
				if ((direct.array() != 0).any()) {
					PhaseFunctionQueryRecord pRec(-ray.d, lRec.d);

					result += throughput * direct * phase->eval(pRec) 
						* scene->evalTransmittance(Ray3f(lRec.ref, lRec.d, 0, lRec.dist), sampler);
				}
	
				/* 2. Recursively sample indirect illumination */
				PhaseFunctionQueryRecord pRec(-ray.d);
				Color3f phaseWeight = phase->sample(pRec, sampler->next2D());
				if ((phaseWeight.array() == 0).all())
					break;
	
				ray = Ray3f(lRec.ref, pRec.wo);
				
				/* Keep track of the throughput along the path */
				throughput *= phaseWeight;
				includeEmitted = false;
			} else {
				throughput *= mediumWeight;

				/* Check if no surface was intersected */
				if (its.t == std::numeric_limits<float>::infinity()) {
					if (includeEmitted && scene->hasEnvLuminaire()) {
						/* Hit an environment luminaire
						   -> query the amount of emitted radiance */
						const Luminaire *env = scene->getEnvLuminaire();
	
						LuminaireQueryRecord lRec(env, ray);
						result += throughput * env->eval(lRec);
					}
					break;
				}
	
				const Mesh *mesh = its.mesh;
				const BSDF *bsdf = mesh->getBSDF();
	
				/* Treat emitted radiance at the first visible surface specially */
				if (includeEmitted && its.mesh->isLuminaire()) {
					const Luminaire *luminaire = its.mesh->getLuminaire();
					LuminaireQueryRecord lRec(luminaire, ray.o, its.p, its.shFrame.n);
					result += throughput * luminaire->eval(lRec);
				}
	
				/* 1. Direct illumination sampling */
				LuminaireQueryRecord lRec(its.p);
				Color3f direct = scene->sampleDirect(lRec, sampler->next2D());
				if ((direct.array() != 0).any()) {
					BSDFQueryRecord bRec(its.toLocal(-ray.d), 
						its.toLocal(lRec.d), ESolidAngle);
	
					result += throughput * direct * bsdf->eval(bRec) 
						* scene->evalTransmittance(Ray3f(lRec.ref, lRec.d, 0, lRec.dist), sampler)
						* std::abs(Frame::cosTheta(bRec.wo));
				}
	
				/* 2. Recursively sample indirect illumination */
				BSDFQueryRecord bRec(its.toLocal(-ray.d));
				Color3f bsdfWeight = bsdf->sample(bRec, sampler->next2D());
				if ((bsdfWeight.array() == 0).all())
					break;
	
				/* Keep track of the throughput and relative 
				   refractive index along the path */
				eta *= bRec.eta;
				throughput *= bsdfWeight;

				if (eta > 2 || eta < 0.5) {
					cout << "OOps!" << endl;
					break;
				}

				ray = Ray3f(its.p, its.shFrame.toWorld(bRec.wo));
	
				/* Include emitted radiance only when having sampled 
				   a Dirac delta BSDF component */
				includeEmitted = bRec.measure == EDiscrete;
			}

			if (++depth > 3) {
				/* Russian roulette: try to keep path weights equal to one,
				   while accounting for the radiance change at refractive index
				   boundaries. Stop with at least some probability to avoid 
				   getting stuck (e.g. due to total internal reflection) */
				float q = std::min(throughput.maxCoeff() * eta * eta, 0.99f);
				if (sampler->next1D() >= q)
					break;
				throughput /= q;
			}
		}

		return result;
	}

	QString toString() const {
		return "PathTracer[]";
	}
private:
	float m_length;
};

NORI_REGISTER_CLASS(PathTracer, "path");
NORI_NAMESPACE_END
