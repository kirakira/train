// CS559 Train Project
// TrainView class implementation
// see the header for details
// look for TODO: to see things you want to add/change
// 

#include "TrainView.H"
#include "TrainWindow.H"

#include "Utilities/3DUtils.H"
#include <iostream>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
#include "GL/gl.h"
#include "GL/glu.h"

#ifdef EXAMPLE_SOLUTION
#include "TrainExample/TrainExample.H"
#endif

using namespace std;


TrainView::TrainView(int x, int y, int w, int h, const char* l) : Fl_Gl_Window(x,y,w,h,l)
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();

	q = gluNewQuadric();
}

void TrainView::resetArcball()
{
	// set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this,40,250,.2f,.4f,0);
}

// FlTk Event handler for the window
// TODO: if you want to make the train respond to other events 
// (like key presses), you might want to hack this.
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		case FL_PUSH:
			last_push = Fl::event_button();
			if (last_push == 1) {
				doPick();
				damage(1);
				return 1;
			};
			break;
		case FL_RELEASE:
			damage(1);
			last_push=0;
			return 1;
		case FL_DRAG:
			if ((last_push == 1) && (selectedCube >=0)) {
				ControlPoint* cp = &world->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x,r1y,r1z, r2x,r2y,r2z);

				double rx, ry, rz;
				mousePoleGo(r1x,r1y,r1z, r2x,r2y,r2z, 
						  static_cast<double>(cp->pos.x), 
						  static_cast<double>(cp->pos.y),
						  static_cast<double>(cp->pos.z),
						  rx, ry, rz,
						  (Fl::event_state() & FL_CTRL) != 0);
				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;
			// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;
		case FL_ENTER:	// every time the mouse enters this window, aggressively take focus
				focus(this);
				break;
		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k=='p') {
					if (selectedCube >=0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",selectedCube,
							world->points[selectedCube].pos.x,world->points[selectedCube].pos.y,world->points[selectedCube].pos.z,
							world->points[selectedCube].orient.x,world->points[selectedCube].orient.y,world->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");
					return 1;
				};
				break;
	}

	return Fl_Gl_Window::handle(event);
}

// this is the code that actually draws the window
// it puts a lot of the work into other routines to simplify things
void TrainView::draw()
{

	glViewport(0,0,w(),h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue
	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	// TODO: you might want to set the lighting up differently
	// if you do, 
	// we need to set up the lights AFTER setting up the projection

	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	} else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}
	// set the light parameters
	GLfloat lightPosition1[] = {0,1,1,0}; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[] = {1, 0, 0, 0};
	GLfloat lightPosition3[] = {0, -1, 0, 0};
	GLfloat yellowLight[] = {0.5f, 0.5f, .1f, 1.0};
	GLfloat whiteLight[] = {1.0f, 1.0f, 1.0f, 1.0};
	GLfloat blueLight[] = {.1f,.1f,.3f,1.0};
	GLfloat grayLight[] = {.3f, .3f, .3f, 1.0};

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);



	// now draw the ground plane
	setupFloor();
	glDisable(GL_LIGHTING);
	drawFloor(200,10);
	glEnable(GL_LIGHTING);
	setupObjects();

	// we draw everything twice - once for real, and then once for
	// shadows
	drawStuff();

	/*
	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}*/
}

// note: this sets up both the Projection and the ModelView matrices
// HOWEVER: it doesn't clear the projection first (the caller handles
// that) - its important for picking
void TrainView::setProjection()
{
	// compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	if (tw->worldCam->value())
		arcball.setProjection(false);
	else if (tw->topCam->value()) {
		float wi,he;
		if (aspect >= 1) {
			wi = 110;
			he = wi/aspect;
		} else {
			he = 110;
			wi = he*aspect;
		}
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glOrtho(-wi,wi,-he,he,-200,200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(90,1,0,0);
	} else {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(120, aspect, .1, 300);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		SplineMethod method = tw->splineMethod();
		Pnt3f zAxis = -world->tangent(method, world->trainU);
		if (zAxis.isZero()) {
			cout << "z axis is zero" << endl;
			return;
		}
		zAxis.normalize();
		Pnt3f xAxis = world->orientation(method, world->trainU) * zAxis;
		if (xAxis.isZero()) {
			cout << "x axis is zero" << endl;
			return;
		}
		xAxis.normalize();
		Pnt3f yAxis = zAxis * xAxis;
		yAxis.normalize();

		Pnt3f pos = world->position(method, world->trainU);

		Pnt3f eye = pos + 20 * yAxis - 10 * zAxis;
		Pnt3f center = pos - 50 * zAxis + 20 * yAxis;
		gluLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z,
			yAxis.x, yAxis.y, yAxis.z);

		// TODO: put code for train view projection here!
#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
}

// this draws all of the stuff in the world
// NOTE: if you're drawing shadows, DO NOT set colors 
// (otherwise, you get colored shadows)
// this gets called twice per draw - once for the objects, once for the shadows
// TODO: if you have other objects in the world, make sure to draw them
void TrainView::drawStuff(bool doingShadows)
{
	drawScenery();

	// draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for(size_t i=0; i<world->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240,60,60);
				else
					glColor3ub(240,240,30);
			}
			world->points[i].draw();
		}
	}
	world->recomputeCache(tw->splineMethod(), trackWidth);

	// draw the track
	drawTrack();

	// draw the train
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain();
}

void TrainView::drawTrack()
{
	if (world->points.size() == 0)
		return;

	bool adaptive = tw->adaptive->value() == 1;
	vector<Pnt3f> leftPoints, rightPoints;

	auto method = tw->splineMethod();
	for (double t = 0, step = .01; t < world->points.size(); t += step) {
		auto tangent = world->tangent(method, t);
		if (tangent.isZero()) {
			cout << "Tangent is zero" << endl;
			continue;
		}

		if (adaptive) {
			double stepLB = .4, stepUB = 7;
			step = (stepUB - stepLB) / pow(1 + 50 * world->curvature(method, t), 3) + stepLB;
		}
		else
			step = 1;
		step /= tangent.norm();

		auto axis = tangent * world->orientation(method, t);
		if (axis.isZero()) {
			cout << "Orientation is parallel with tangent" << endl;
			continue;
		}
		axis.normalize();

		auto pos = world->position(method, t);
		auto right = pos + trackWidth / 2 * axis,
			left = pos + -trackWidth / 2 * axis;

		leftPoints.emplace_back(left);
		rightPoints.emplace_back(right);

		/*
		glVertex3f(left.x, left.y, left.z);
		glVertex3f(right.x, right.y, right.z);*/
		/*leftPoints.emplace_back(world->leftTrackPoint(method, t, trackWidth));
		rightPoints.emplace_back(world->rightTrackPoint(method, t, trackWidth));*/
	}

	glLineWidth(10);
	glColor3d(.1, .1, .1);
	glBegin(GL_LINE_LOOP);
	for (const auto& p : leftPoints)
		glVertex3f(p.x, p.y, p.z);
	glEnd();

	glBegin(GL_LINE_LOOP);
	for (const auto& p : rightPoints)
		glVertex3f(p.x, p.y, p.z);
	glEnd();

	const double tieGap = 13;
	glColor3f(.45, .45, .45);
	for (double t = 0; t < world->points.size(); ) {
		auto mid = world->position(method, t);
		auto orient = world->orientation(method, t);
		auto tangent = world->tangent(method, t);
		auto side = tangent * orient;
		if (side.isZero())
			cout << "Orient is parallel to tangent" << endl;
		else {
			tangent.normalize();
			orient.normalize();
			side.normalize();

			glPushMatrix();
			double tie[] = {
				side.x, side.y, side.z, 0,
				orient.x, orient.y, orient.z, 0,
				-tangent.x, -tangent.y, -tangent.z, 0,
				mid.x, mid.y, mid.z, 1
			};
			glMultMatrixd(tie);
			glTranslated(-.8 * trackWidth, -1, -1);
			drawCuboid(1.6 * trackWidth, 2, 2);
			glPopMatrix();
		}
		t = arclenNextT(t, tieGap);
	}
}

void TrainView::drawScenery()
{
	glPushMatrix();
	glTranslated(90, 0, 90);
	drawTree();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-90, 0, 90);
	drawTree();
	glPopMatrix();

	glPushMatrix();
	glTranslated(90, 0, -90);
	drawTree();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-90, 0, -90);
	drawTree();
	glPopMatrix();
}

void TrainView::drawTree()
{
	const double height = 20;
	glPushMatrix();
	glColor3f(0.359, .25, .199);
	glRotated(-90, 1, 0, 0);
	drawSolidCylinder(2, 2, height);
	glPopMatrix();

	glPushMatrix();
	glColor3f(0, .39, 0);
	glTranslated(0, height, 0);
	glutSolidSphere(8, 100, 100);
	glPopMatrix();
}

// this tries to see which control point is under the mouse
// (for when the mouse is clicked)
// it uses OpenGL picking - which is always a trick
// TODO: if you want to pick things other than control points, or you
// changed how control points are drawn, you might need to change this
void TrainView::doPick()
{
	make_current();		// since we'll need to do some GL stuff

	int mx = Fl::event_x(); // where is the mouse?
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	// set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<world->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		world->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}

double TrainView::arclenNextT(double t, double dt) const
{
	SplineMethod method = tw->splineMethod();
	double sign = dt > 0 ? 1 : -1;
	while (dt * sign > 0) {
		auto tan = world->tangent(method, t);
		double curr, step = min(.5, abs(dt));
		if (tan.isZero()) {
			cout << "tangent is zero!" << endl;
			curr = step;
		} else
			curr = step / tan.norm();
		t += sign * curr;
		dt -= sign * step;
	}
	return t;
}

vector<double> TrainView::computeCarPositions() const
{
	SplineMethod method = tw->splineMethod();
	vector<double> ret = { world->trainU };
	double t = world->trainU;
	for (int i = 0; i < nTrains; ++i) {
		t = arclenNextT(t, -(i == 0 ? 27 : 23));
		ret.emplace_back(t);
	}
	return ret;
}

void TrainView::drawTrain()
{
	bool first = true;
	for (auto t : computeCarPositions()) {
		glPushMatrix();
		moveOriginTo(t);
		if (first) {
			drawTrainHead();
			first = false;
		}
		else
			drawTrainMiddle();
		glPopMatrix();
	}
}

void TrainView::moveOriginTo(double t)
{
	SplineMethod method = tw->splineMethod();
	Pnt3f zAxis = -world->tangent(method, t);
	if (zAxis.isZero()) {
		cout << "z axis is zero" << endl;
		return;
	}
	zAxis.normalize();
	Pnt3f xAxis = world->orientation(method, t) * zAxis;
	if (xAxis.isZero()) {
		cout << "x axis is zero" << endl;
		return;
	}
	xAxis.normalize();
	Pnt3f yAxis = zAxis * xAxis;
	yAxis.normalize();

	Pnt3f pos = world->position(method, t);

	double coord[] = { xAxis.x, xAxis.y, xAxis.z, 0,
		yAxis.x, yAxis.y, yAxis.z, 0,
		zAxis.x, zAxis.y, zAxis.z, 0,
		pos.x, pos.y, pos.z, 1 };
	glMultMatrixd(coord);
}

void TrainView::drawTrainMiddle()
{
	glColor3f(0, 0, 1);
	glPushMatrix();
	glTranslated(-5, 3, -10);
	drawCuboid(10, 5, 20);
	glPopMatrix();

	glPushMatrix();
	glTranslated(-5, 8, 0);
	drawCuboid(10, 3, 10);
	glPopMatrix();

	glColor3f(0, 0, 0);
	glPushMatrix();
	glTranslated(0, 0, -6);
	glRotated(90, 0, 1, 0);
	glTranslated(0, 2, -5);
	drawSolidCylinder(2, 2, 10);
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 0, 6);
	glRotated(90, 0, 1, 0);
	glTranslated(0, 2, -5);
	drawSolidCylinder(2, 2, 10);
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 8, -4);
	drawMan(angle(world->tangent(tw->splineMethod(), world->trainU), Pnt3f(0, -1, 0)) < 75);
	glPopMatrix();
}

void TrainView::drawMan(bool handsUp)
{
	glPushMatrix();
	glRotated(-90, 1, 0, 0);
	glColor3f(.6, 0, 0);
	drawSolidCylinder(3, 3, 5);
	glPopMatrix();

	glPushMatrix();
	glColor3f(.89, .723, .554);
	glTranslated(0, 8, 0);
	gluSphere(q, 3, 100, 100);
	glPopMatrix();

	glPushMatrix();
	glColor3f(.799, .516, .262);
	glTranslated(-4, handsUp ? 4 : 0, 0);
	glRotated(handsUp ? -120 : -90, 1, 0, 0);
	drawSolidCylinder(1, 1, 5);
	glPopMatrix();

	glPushMatrix();
	glColor3f(.799, .516, .262);
	glTranslated(4, handsUp ? 4 : 0, 0);
	glRotated(handsUp ? -120 : -90, 1, 0, 0);
	drawSolidCylinder(1, 1, 5);
	glPopMatrix();
}

// draw a cuboid that extends [0, x] * [0, y] * [0, z]
void TrainView::drawCuboid(double x, double y, double z)
{
	// front
	glRectd(0, 0, x, y);
	// back
	glPushMatrix();
	glTranslated(0, 0, z);
	glRectd(0, 0, x, y);
	glPopMatrix();

	// left
	glPushMatrix();
	glRotated(-90, 0, 1, 0);
	glRectd(0, 0, z, y);
	// right
	glTranslated(0, 0, -x);
	glRectd(0, 0, z, y);
	glPopMatrix();

	// top
	glPushMatrix();
	glRotated(90, 1, 0, 0);
	glRectd(0, 0, x, z);
	// bottom
	glTranslated(0, 0, -y);
	glRectd(0, 0, x, z);
	glPopMatrix();
}

// the train is oriented towards -z, wheels touching plane y=0, middle of body at x=0
void TrainView::drawTrainHead()
{
	double body_radius = 6, body_length = 30;

	glPushMatrix();
	glTranslated(0, -3, -body_length / 2);

	// body
	glPushMatrix();
	glColor3f(1, .5, .5);
	glTranslated(0, 10, 0);
	drawSolidCylinder(body_radius, body_radius, body_length);
	glPopMatrix();

	// wheels
	glPushMatrix();
	glColor3f(.2, .2, .2);
	double wheel_length = 2 * body_radius;
	glTranslated(-wheel_length / 2, 6, body_length * .2);
	glRotated(90, 0, 1, 0);
	drawSolidCylinder(3, 3, wheel_length);
	glPopMatrix();

	glPushMatrix();
	glColor3f(.2, .2, .2);
	glTranslated(-wheel_length / 2, 6, body_length * .8);
	glRotated(90, 0, 1, 0);
	drawSolidCylinder(3, 3, wheel_length);
	glPopMatrix();

	// room
	glPushMatrix();
	glColor3f(.2, 1, .2);
	glTranslated(0, 8, body_length * .75);
	glRotated(-90, 1, 0, 0);
	drawSolidCylinder(body_radius * .95, body_radius * .95, body_length * .55);
	glPopMatrix();

	glPushMatrix();
	glColor3f(.2, .2, 1);
	glTranslated(0, 12, body_length * .3);
	glRotated(-90, 1, 0, 0);
	drawSolidCylinder(body_radius * .4, body_radius * .45, body_length / 4);
	glPopMatrix();

	glPopMatrix();
}

void TrainView::drawSolidCylinder(double base, double top, double height)
{
	gluCylinder(q, base, top, height, 100, 100);
	gluDisk(q, 0, base, 100, 100);
	glPushMatrix();
	glTranslated(0, 0, height);
	gluDisk(q, 0, top, 100, 100);
	glPopMatrix();
}

// CVS Header - if you don't know what this is, don't worry about it
// This code tells us where the original came from in CVS
// Its a good idea to leave it as-is so we know what version of
// things you started with
// $Header: /p/course/cs559-gleicher/private/CVS/TrainFiles/TrainView.cpp,v 1.10 2009/11/08 21:34:13 gleicher Exp $
