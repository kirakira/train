#pragma once
// CS559 Train Project
//	begun Oct 2008, Mike Gleicher
//
// Container for the "World"
//
// This provides a container for all of the "stuff" in the world.
//
// It could have been all global variables, or it could have just been
// contained in the window. The advantage of doing it this way is that
// we might have multiple windows looking at the same world. But, I don't
// think we'll actually do that.
//
// See the readme for commentary on code style
#include <vector>
#include <map>

// avoid having to say std::vector all of the time
using std::vector;

// make use of other data structures from this project
#include "ControlPoint.H"

enum SplineMethod {
	LINEAR,
	CARDINAL_CUBIC_SPLINE,
	CARDINAL_B_SPLINE
};

class World {
public:		// we don't make anything private - for no good reason
	World();

	// rather than have generic objects, we make a special case for these few
	// objects that we know that all implementations are going to need and that
	// we're going to have to handle specially
	vector<ControlPoint> points;

	// when we want to clear the control points, we really "reset" them 
	// to have 4 default positions (since we should never have fewer
	// than 4 points)
	void resetPoints();

	// the state of the train - basically, all I need to remember is where
	// it is in parameter space
	// TODO: you might want to do this differently
	float trainU;

	// read and write to files
	void readPoints(const char* filename);
	void writePoints(const char* filename);

	Pnt3f leftTrackPoint(SplineMethod m, double t, double trackWidth);
	Pnt3f rightTrackPoint(SplineMethod m, double t, double trackWidth);
	Pnt3f position(SplineMethod m, double t);
	Pnt3f orientation(SplineMethod m, double t);
	Pnt3f tangent(SplineMethod m, double t);
	double curvature(SplineMethod m, double t);

	double tension = 0;

	void recomputeCache(SplineMethod s, double trackWidth);

private:
	std::vector<int> adjacentPoints(double t);
	Pnt3f interpolate(SplineMethod s, double t,
		const Pnt3f& i_prev, const Pnt3f& i, const Pnt3f& j, const Pnt3f& j_next);
	Pnt3f interpolateTangent(SplineMethod s, double t,
		const Pnt3f& i_prev, const Pnt3f& i, const Pnt3f& j, const Pnt3f& j_next);
	Pnt3f interpolateSecondDeriv(SplineMethod s, double t,
		const Pnt3f& i_prev, const Pnt3f& i, const Pnt3f& j, const Pnt3f& j_next);

	double translateToUnitInterval(double t);

	std::vector<Pnt3f> leftTrackCache, rightTrackCache;
};

// $Header: /p/course/cs559-gleicher/private/CVS/TrainFiles/World.H,v 1.5 2008/10/16 21:15:16 gleicher Exp $