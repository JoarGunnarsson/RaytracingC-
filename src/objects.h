#ifndef OBJECTS_H
#define OBJECTS_H
#include "vec3.h"
#include "utils.h"
#include "materials.h"
#include "constants.h"
#include "colors.h"


class Object{
    public:
        Material* material;
        double area;
        Object(){}
        Object(Material* _material){
            material = _material;
        }

        virtual vec3 getUV(const vec3& hitPoint){
            throw VirtualMethodNotAllowedException("this is a pure virtual method and should not be called.");
            vec3 vec;
            return vec;
        }

        virtual vec3 eval(const vec3& intersectionPoint){
            vec3 UV = getUV(intersectionPoint);
            return material -> eval(UV[0], UV[1]);
        }

        virtual brdfData sample(const Hit& hit, Object** objectPtrList, const int numberOfObjects){
            vec3 UV = getUV(hit.intersectionPoint);
            return material -> sample(hit, objectPtrList, numberOfObjects, UV[0], UV[1]);
        }

        virtual vec3 getLightEmittance(const vec3& intersectionPoint){
            vec3 UV = getUV(intersectionPoint);
            return material -> getLightEmittance(UV[0], UV[1]);
        }

        virtual Hit findClosestHit(const Ray& ray){
            throw VirtualMethodNotAllowedException("this is a pure virtual method and should not be called.");
            Hit hit;
            return hit;
        }
        
        virtual vec3 getNormalVector(const vec3& intersectionPoint){
            throw VirtualMethodNotAllowedException("this is a pure virtual method and should not be called.");
            vec3 vec;
            return vec;
        }

        virtual vec3 generateRandomSurfacePoint(){
            throw VirtualMethodNotAllowedException("this is a pure virtual method and should not be called.");
            vec3 point;
            return point;
        }

        virtual vec3 randomLightPoint(const vec3& referencePoint, double& inversePDF){
            throw VirtualMethodNotAllowedException("this is a pure virtual method and should not be called.");
            vec3 point;
            return point;
        }

        double areaToAnglePDFFactor(const vec3& surfacePoint, const vec3& referencePoint){
            vec3 normalVector = getNormalVector(surfacePoint);
            vec3 differenceVector = referencePoint - surfacePoint;
            vec3 vectorToPoint = normalizeVector(differenceVector);
            double inversePDF = dotVectors(normalVector, vectorToPoint) / differenceVector.length_squared();
            return std::max(0.0, inversePDF);
        }
};


class Sphere: public Object{
    public:
        vec3 position;
        double radius;
        double radiusSquared;
        Sphere(){}
        Sphere(vec3 _position, double _radius, Material* _material) : Object(_material){
            position = _position;
            radius = _radius;
            area = 4 * M_PI * radius * radius;
            radiusSquared = radius * radius;
        }

        vec3 getUV(const vec3& hitPoint) override{
            vec3 unitSpherePoint = (hitPoint - position) / radius;
            double x = -unitSpherePoint[0];
            double y = -unitSpherePoint[1];
            double z = -unitSpherePoint[2];
            double u = 0.5 + atan2(z, x) / (2 * M_PI);
            double v = 0.5 + asin(y) / (M_PI);
            return vec3(u, v, 0);
        }

        Hit findClosestHit(const Ray& ray) override{

            double dotProduct = dotVectors(ray.directionVector, ray.startingPosition);
            double b = 2 * (dotProduct - dotVectors(ray.directionVector, position));
            vec3 difference_in_positions = position - ray.startingPosition;
            double c = difference_in_positions.length_squared() - radiusSquared;
            double distance = solveQuadratic(b, c);
            Hit hit;
            hit.objectID = 0;
            hit.distance = distance;
            return hit;
        }

        vec3 getNormalVector(const vec3& intersectionPoint) override{
            vec3 differenceVector = intersectionPoint - position;
            return normalizeVector(differenceVector);
        }

        vec3 generateRandomSurfacePoint() override{
            return sampleSpherical() * radius + position;
        }

        vec3 randomLightPoint(const vec3& referencePoint, double& inversePDF) override{
            double distance = (referencePoint - position).length();
            if (distance <= radius){
                vec3 randomPoint = generateRandomSurfacePoint();
                inversePDF = areaToAnglePDFFactor(randomPoint, referencePoint) * area;
                return randomPoint;
            }
        
        double cosThetaMax = sqrt(1 - pow(radius / distance, 2));
        inversePDF = 2 * M_PI * (1 - (cosThetaMax));

        double rand = randomUniform(0, 1);
        double cosTheta = 1 + rand * (cosThetaMax-1);
        double sinTheta = sqrt(1 - cosTheta * cosTheta);
        double cosAlpha = (radiusSquared + distance * distance - pow(distance * cosTheta - sqrt(radiusSquared - pow(distance*sinTheta, 2)), 2)) / (2.0 * distance * radius);
        double sinAlpha = sqrt(1.0 - cosAlpha * cosAlpha);
        
        vec3 xHat;
        vec3 yHat;
        vec3 zHat = getNormalVector(referencePoint);
        setPerpendicularVectors(zHat, xHat, yHat);
        double phi = randomUniform(0, 2.0*M_PI);
        vec3 randomPoint = xHat * sinAlpha * cos(phi) + yHat * sinAlpha * sin(phi) + zHat * cosAlpha;
        return randomPoint * radius + position;
        }
};


class Plane: public Object{
    public:
        vec3 position;
        vec3 v1;
        vec3 v2;
        vec3 normalVector;
        bool transparentBack;
        Plane(){}
        Plane(vec3 _position, vec3 _v1, vec3 _v2, Material* _material) : Object(_material){
            position = _position;
            v1 = normalizeVector(_v1);
            v2 = normalizeVector(_v2);
            vec3 _normalVector = crossVectors(v1, v2);
            normalVector = normalizeVector(_normalVector);
        }

        vec3 getUV(const vec3& hitPoint) override{
            vec3 shiftedPoint = hitPoint - position;
            double u = 1 - dotVectors(shiftedPoint, v1) - 0.5;
            double v = 1 - dotVectors(shiftedPoint, v2) - 0.5;
            return vec3(u, v, 0);
        }

        double computeDistanceInCenteredSystem(const vec3& startingPoint, const vec3& directionVector){
            double directionDotNormal = -dotVectors(directionVector, normalVector);
            if (std::abs(directionDotNormal) < constants::EPSILON){
                return -1;
            }

            double distancesToStart = dotVectors(startingPoint, normalVector);
            return distancesToStart / directionDotNormal;
        }

        Hit findClosestHit(const Ray& ray) override{
            vec3 shiftedPoint = ray.startingPosition - position;
            double distance = computeDistanceInCenteredSystem(shiftedPoint, ray.directionVector);
            Hit hit;
            hit.objectID = 0;
            hit.distance = distance;
            return hit;
        }

        vec3 getNormalVector(const vec3& intersectionPoint) override{
            return normalVector;
        }

};


class Rectangle: public Plane{
    public:
        double L1;
        double L2;
        Rectangle(){}
        Rectangle(vec3 _position, vec3 _v1, vec3 _v2, double _L1, double _L2, Material* _material) : Plane(_position, _v1, _v2, _material){
            L1 = _L1;
            L2 = _L2;
            area = L1 * L2;
        }
        vec3 getUV(const vec3& hitPoint) override{
            vec3 shiftedPoint = hitPoint - position;
            double u = 1 - dotVectors(shiftedPoint, v1) / L1 - 0.5;
            double v = 1 - dotVectors(shiftedPoint, v2) / L2 - 0.5;
            return vec3(u, v, 0);
        }

        Hit findClosestHit(const Ray& ray) override{
            Hit hit;
            hit.objectID = 0;
            hit.distance = -1;

            vec3 shiftedPoint = ray.startingPosition - position;
            double distance = Plane::computeDistanceInCenteredSystem(shiftedPoint, ray.directionVector);
            if (distance < 0){
                return hit;
            }
            double directionDotV1 = dotVectors(ray.directionVector, v1);
            double directionDotV2 = dotVectors(ray.directionVector, v2);
            double startDotV1 = dotVectors(shiftedPoint, v1);
            double startDotV2 = dotVectors(shiftedPoint, v2);

            if (std::abs(startDotV1 + directionDotV1 * distance) > L1 / 2.0 + constants::EPSILON || std::abs(startDotV2 + directionDotV2 * distance) > L2 / 2.0 + constants::EPSILON){
                return hit;
            }
            hit.distance = distance;
            return hit;
        }

        vec3 generateRandomSurfacePoint() override{
            double r1 = randomUniform(-L1/2, L1/2);
            double r2 = randomUniform(-L2/2, L2/2);
            return v1 * r1 + v2 * r2 + position;
        }

        vec3 randomLightPoint(const vec3& referencePoint, double& inversePDF) override{
            vec3 randomPoint = generateRandomSurfacePoint();
            inversePDF = area * areaToAnglePDFFactor(randomPoint, referencePoint);
            return randomPoint;
        }
};


class Triangle: public Object{
    public:
        vec3 position;
        vec3 normalVector;
        vec3 p1;
        vec3 p2;
        vec3 p3;
        vec3 v1;
        vec3 v2;
        double x1;
        double y1;
        double x2;
        double y2;
        double x3;
        double y3;
        double detT;

        vec3 uv1;
        vec3 uv2;
        vec3 uv3;

        Triangle(vec3 _p1, vec3 _p2, vec3 _p3, Material* _material) : Object(_material){
            p1 = _p1;
            p2 = _p2;
            p3 = _p3;

            position = p1;

            v1 = normalizeVector(p2 - p1);
            v2 = normalizeVector(p3 - p1);
            normalVector = normalizeVector(crossVectors(v1, v2));
            v2 = normalizeVector(crossVectors(normalVector, v1));

            x1 = dotVectors(p1, v1);
            y1 = dotVectors(p1, v2);
            x2 = dotVectors(p2, v1);
            y2 = dotVectors(p2, v2);
            x3 = dotVectors(p3, v1);
            y3 = dotVectors(p3, v2);
            detT = (y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3);

            area = 0.5 * std::abs(x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));

            uv1 = vec3(0, 0, 0);
            uv2 = vec3(1, 0, 0);
            uv3 = vec3(0, 1, 0);
        }

        vec3 getNormalVector(const vec3& point) override{
            return normalVector;
        }

        vec3 computeBarycentric(const vec3& point){
            double x = dotVectors(point, v1);
            double y = dotVectors(point, v2);

            double lambda1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / detT;
            double lambda2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / detT;
            return vec3(lambda1, lambda2, 1 - lambda1 - lambda2);
        }

        vec3 getUV(const vec3& hitPoint) override{
            vec3 barycentricVector = computeBarycentric(hitPoint);
            return uv1 * barycentricVector[0] + uv2 * barycentricVector[1] + uv3 * barycentricVector[2];
        }

        Hit findClosestHit(const Ray& ray) override{
            Hit hit;
            hit.objectID = 0;
            hit.distance = -1;
            vec3 shiftedPoint = ray.startingPosition - position;

            double directionDotNormal = -dotVectors(ray.directionVector, normalVector);
            if (std::abs(directionDotNormal) < constants::EPSILON){
                return hit;
            }

            double distancesToStart = dotVectors(shiftedPoint, normalVector);
            double distance = distancesToStart / directionDotNormal;

            if (distance < 0){
                return hit;
            }

            vec3 inPlanePoint = ray.startingPosition + ray.directionVector * distance;

            vec3 barycentricVector = computeBarycentric(inPlanePoint);
            if (barycentricVector[0] < 0 || barycentricVector[1] < 0 || barycentricVector[2] < 0){
                return hit;
            }
            hit.distance = distance;
            return hit;
        }

        vec3 generateRandomSurfacePoint() override{
            double r1 = randomUniform(0, 1);
            double r2 = randomUniform(0, 1);
            return p1 * (1.0 - sqrt(r1)) + p2 * (sqrt(r1) * (1.0 - r2)) + p3 * (sqrt(r1) * r2);
        }

        vec3 randomLightPoint(const vec3& referencePoint, double& inversePDF) override{
            vec3 randomPoint = generateRandomSurfacePoint();
            inversePDF = area *  areaToAnglePDFFactor(randomPoint, referencePoint);
            return randomPoint;
        }
};


Hit findClosestHit(const Ray& ray, Object** objects, const int size){
    Hit closestHit;
    closestHit.distance = -1;
    for (int i = 0; i < size; i++){
        Hit hit = objects[i] -> findClosestHit(ray);
        if (hit.distance > constants::EPSILON && (hit.distance < closestHit.distance || closestHit.distance == -1)){
            hit.intersectedObjectIndex = i;
            closestHit = hit;
        }
    }
    if (closestHit.distance < constants::EPSILON){
        return closestHit;
    }

    closestHit.intersectionPoint = ray.startingPosition + ray.directionVector * closestHit.distance;
    closestHit.normalVector = objects[closestHit.intersectedObjectIndex] -> getNormalVector(closestHit.intersectionPoint);
    closestHit.incomingVector = ray.directionVector;
    return closestHit;
 }


#endif