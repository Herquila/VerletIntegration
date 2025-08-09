#ifndef __VERLET_H__
#define __VERLET_H__

#include "mathc.h"

#define CONTAINER_RADIUS 6.0f
#define VERLET_RADIUS 0.15f

#include "uthash.h"
#include <stdbool.h>



typedef enum {
    RED,
    GREEN,
    BLUE,
    WHITE,
    //INVISIBLE,
    // Add more colors as needed
} ParticleColor;

typedef struct {
    mfloat_t current[VEC3_SIZE];
    mfloat_t previous[VEC3_SIZE];
    mfloat_t acceleration[VEC3_SIZE];
    mfloat_t radius;
    ParticleColor color;
    mfloat_t colorVector[VEC3_SIZE]; // Add a color vector to store RGB values
    bool visible;
    // Mass of the particle (depends on color)
    mfloat_t mass;
} VerletObject;

struct NodeStruct {
    VerletObject* val;
    struct NodeStruct* next;
};
typedef struct NodeStruct Node;


void setColorVector(VerletObject* obj);
// Initialize mass based on the particle color
void setMassFromColor(VerletObject* obj);
void applyForces(VerletObject* objects, int size);
void applyCollisions(VerletObject* objects, int size);
void applyConstraints(VerletObject* objects, int size, mfloat_t* containerPosition);
void updatePositions(VerletObject* objects, int size, float dt);

void clearGrid();
void fillGrid(VerletObject* objects, int size);
void applyGridCollisions(VerletObject* objects, int size);

void addForce(VerletObject* objects, int size, mfloat_t* center, float strength);

#endif