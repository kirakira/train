#include "World.H"

#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <FL/fl_ask.h>

using namespace std;

World::World() : trainU(0)
{
	resetPoints();
}

// provide a default set of points
void World::resetPoints()
{
	points.clear();
	points.push_back(ControlPoint(Pnt3f(50,5,0)));
	points.push_back(ControlPoint(Pnt3f(0,5,50)));
	points.push_back(ControlPoint(Pnt3f(-50,5,0)));
	points.push_back(ControlPoint(Pnt3f(0,5,-50)));

	// we had better put the train back at the start of the track...
	trainU = 0.0;
}

// handy utility to break a string into a list of words
// this originally came from an old 559 project a long time ago,
// and now appears in just about every program I write
void breakString(char* str, vector<const char*>& words) {
	// start with no words
	words.clear();

	// scan through the string, starting at the beginning
	char* p = str;

	// stop when we hit the end of the string
	while(*p) {
		// skip over leading whitespace - stop at the first character or end of string
		while (*p && *p<=' ') p++;

		// now we're pointing at the first thing after the spaces
		// make sure its not a comment, and that we're not at the end of the string
		// (that's actually the same thing)
		if (! (*p) || *p == '#')
		break;

		// so we're pointing at a word! add it to the word list
		words.push_back(p);

		// now find the end of the word
		while(*p > ' ') p++;	// stop at space or end of string

		// if its ethe end of the string, we're done
		if (! *p) break;

		// otherwise, turn this space into and end of string (to end the word)
		// and keep going
		*p = 0;
		p++;
	}
}

// the file format is simple
// first line: an integer with the number of control points
// other lines: one line per control point
// either 3 (X,Y,Z) numbers on the line, or 6 numbers (X,Y,Z, orientation)
void World::readPoints(const char* filename)
{
	FILE* fp = fopen(filename,"r");
	if (!fp) {
		fl_alert("Can't Open File!\n");
	} else {
		char buf[512];

		// first line = number of points
		fgets(buf,512,fp);
		size_t npts = (size_t) atoi(buf);

		if( (npts<4) || (npts>65535)) {
			fl_alert("Illegal Number of Points Specified in File");
		} else {
			points.clear();
			// get lines until EOF or we have enough points
			while( (points.size() < npts) && fgets(buf,512,fp) ) {
				Pnt3f pos,orient;
				vector<const char*> words;
				breakString(buf,words);
				if (words.size() >= 3) {
					pos.x = (float) strtod(words[0],0);
					pos.y = (float) strtod(words[1],0);
					pos.z = (float) strtod(words[2],0);
				} else {
					pos.x=0;
					pos.y=0;
					pos.z=0;
				}
				if (words.size() >= 6) {
					orient.x = (float) strtod(words[3],0);
					orient.y = (float) strtod(words[4],0);
					orient.z = (float) strtod(words[5],0);
				} else {
					orient.x = 0;
					orient.y = 1;
					orient.z = 0;
				}
				orient.normalize();
				points.push_back(ControlPoint(pos,orient));
			}
		}
		fclose(fp);
	}
	trainU = 0;
}

// write the control points to our simple format
void World::writePoints(const char* filename)
{
	FILE* fp = fopen(filename,"w");
	if (!fp) {
		fl_alert("Can't open file for writing");
	} else {
		fprintf(fp,"%d\n",points.size());
		for(size_t i=0; i<points.size(); ++i)
			fprintf(fp,"%g %g %g %g %g %g\n",
				points[i].pos.x, points[i].pos.y, points[i].pos.z, 
				points[i].orient.x, points[i].orient.y, points[i].orient.z);
		fclose(fp);
	}
}

vector<int> World::adjacentPoints(double t)
{
	int n = points.size();
	t -= floor(t / n) * n;
	int i = (int)t;
	return { (i + n - 1) % n, i, (i + 1) % n, (i + 2) % n };
}

Pnt3f World::interpolate(SplineMethod s, double t, const Pnt3f& i_prev, const Pnt3f& i, const Pnt3f& j, const Pnt3f& j_next)
{
	if (s == LINEAR)
		return (1 - t) * i + t * j;
	else if (s == CARDINAL_CUBIC_SPLINE) {
		auto m_i = (1 - tension) / 2 * (j + -1 * i_prev),
			m_j = (1 - tension) / 2 * (j_next + -1 * i);
		return (2 * t * t * t - 3 * t * t + 1) * i + (t * t * t - 2 * t * t + t) * m_i
			+ (-2 * t * t * t + 3 * t * t) * j + (t * t * t - t * t) * m_j;
	} else if (s == CARDINAL_B_SPLINE) {
		return (1 - t) * (1 - t) * (1 - t) / 6 * i_prev + (3 * t * t * t - 6 * t * t + 4) / 6 * i
			+ (-3 * t * t * t + 3 * t * t + 3 * t + 1) / 6 * j + (t * t * t) / 6 * j_next;
	} else
		return Pnt3f();
}

Pnt3f World::interpolateTangent(SplineMethod s, double t, const Pnt3f& i_prev, const Pnt3f& i, const Pnt3f& j, const Pnt3f& j_next)
{
	if (s == LINEAR)
		return -1 * i + j;
	else if (s == CARDINAL_CUBIC_SPLINE) {
		auto m_i = (1 - tension) / 2 * (j + -1 * i_prev),
			m_j = (1 - tension) / 2 * (j_next + -1 * i);
		return (6 * t * t - 6 * t) * i + (3 * t * t - 4 * t + 1) * m_i
			+ (-6 * t * t + 6 * t) * j + (3 * t * t - 2 * t) * m_j;
	} else if (s == CARDINAL_B_SPLINE) {
		return -(1 - t) * (1 - t) / 2 * i_prev + (9 * t * t - 12 * t) / 6 * i
			+ (-9 * t * t + 6 * t + 3) / 6 * j + (3 * t * t) / 6 * j_next;
	}
	else
		return Pnt3f();
}

Pnt3f World::interpolateSecondDeriv(SplineMethod s, double t,
	const Pnt3f& i_prev, const Pnt3f& i, const Pnt3f& j, const Pnt3f& j_next)
{
	if (s == LINEAR)
		return Pnt3f(0, 0, 0);
	else if (s == CARDINAL_CUBIC_SPLINE) {
		auto m_i = (1 - tension) / 2 * (j + -1 * i_prev),
			m_j = (1 - tension) / 2 * (j_next + -1 * i);
		return (12 * t - 6) * i + (6 * t - 4) * m_i
			+ (-12 * t + 6) * j + (6 * t - 2) * m_j;
	}
	else if (s == CARDINAL_B_SPLINE) {
		return (1 - t) * i_prev + (3 * t - 2) * i
			+ (-3 * t + 1) * j + t * j_next;
	}
	else
		return Pnt3f();
}

Pnt3f World::position(SplineMethod s, double t)
{
	auto interval = adjacentPoints(t);
	return
		interpolate(s, translateToUnitInterval(t), points[interval[0]].pos, points[interval[1]].pos,
		points[interval[2]].pos, points[interval[3]].pos);
}

Pnt3f World::tangent(SplineMethod s, double t)
{
	auto interval = adjacentPoints(t);
	return
		interpolateTangent(s, translateToUnitInterval(t), points[interval[0]].pos, points[interval[1]].pos,
		points[interval[2]].pos, points[interval[3]].pos);
}

double World::curvature(SplineMethod s, double t)
{
	auto interval = adjacentPoints(t);
	auto tangent = interpolateTangent(s, translateToUnitInterval(t), points[interval[0]].pos, points[interval[1]].pos,
		points[interval[2]].pos, points[interval[3]].pos);
	if (tangent.isZero()) {
		cout << "Tangent is zero!" << endl;
		return 0;
	}
	double denum = tangent.norm() * tangent.norm();
	return
		interpolateSecondDeriv(s, translateToUnitInterval(t),
		points[interval[0]].pos, points[interval[1]].pos,
		points[interval[2]].pos, points[interval[3]].pos).norm() / denum;
}

void World::recomputeCache(SplineMethod s, double trackWidth)
{
	leftTrackCache.clear();
	rightTrackCache.clear();
	int n = points.size();
	for (int i = 0; i < n; ++i) {
		auto pitchAxis = tangent(s, i) * points[i].orient;
		Pnt3f left, right;
		if (pitchAxis.isZero()) {
			cout << "Orient is parallel to tangent" << endl;
		}
		else {
			pitchAxis.normalize();
			left = points[i].pos + trackWidth / 2 * pitchAxis;
			right = points[i].pos + -trackWidth / 2 * pitchAxis;
		}
		leftTrackCache.emplace_back(left);
		rightTrackCache.emplace_back(right);
	}
}

Pnt3f World::orientation(SplineMethod s, double t)
{
	auto interval = adjacentPoints(t);
	return
		interpolate(s, translateToUnitInterval(t), points[interval[0]].orient, points[interval[1]].orient,
		points[interval[2]].orient, points[interval[3]].orient);
}

Pnt3f World::leftTrackPoint(SplineMethod s, double t, double trackWidth)
{
	auto interval = adjacentPoints(t);
	return interpolate(s, translateToUnitInterval(t),
		leftTrackCache[interval[0]],
		leftTrackCache[interval[1]],
		leftTrackCache[interval[2]],
		leftTrackCache[interval[3]]);
}

Pnt3f World::rightTrackPoint(SplineMethod s, double t, double trackWidth)
{
	auto interval = adjacentPoints(t);
	return interpolate(s, translateToUnitInterval(t),
		rightTrackCache[interval[0]],
		rightTrackCache[interval[1]],
		rightTrackCache[interval[2]],
		rightTrackCache[interval[3]]);
}

double World::translateToUnitInterval(double t)
{
	return t - floor(t);
}

// CVS Header - if you don't know what this is, don't worry about it
// This code tells us where the original came from in CVS
// Its a good idea to leave it as-is so we know what version of
// things you started with
// $Header: /p/course/cs559-gleicher/private/CVS/TrainFiles/World.cpp,v 1.5 2008/10/19 01:54:28 gleicher Exp $
