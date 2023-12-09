#include <movement.cpp>
#include <Arduino.h>

#define DISABLE_COMPLEX_FUNCTIONS
#define ENABLE_EASE_CUBIC

#include <ServoEasing.hpp>

#define FRONT 1 // if ever change these values(pls dont), change legs function in body
#define MID   2
#define BACK  3
#define HIP   3
#define KNEE  5
#define ANKLE 7
#define LEFT  -1
#define RIGHT 1

#define SERVO_SPEED 3 // milliseconds per degree: currently placeholder

// Ellipse constants
#define ELLIPSE_SPEED 1
#define LR_ELLIPSE G_CIRCLE_RADIUS
#define SR_ELLIPSE 3

#define TURNING_ANGLE 9
// custom servo wrapper class
class Joint {
  public:
    int id; 
    int rangeDown; 
    int rangeUp; // need to know the actual mobility range of the servo
    double prev; // storing the last sent signal
    int flatAngle;
    ServoEasing servo;

    Joint(int id, int info[4])
    : id(id), rangeDown(info[3]), rangeUp(info[2]), flatAngle(info[1]) {
        servo.attach(info[0]);
        setSpeedForAllServos(500);
        prev = 0; // can change this doesn't matter
        // write(flatAngle);
    }

    int write(double angle) {
        angle = flatAngle + angle;
        if (angle < rangeDown) {
            angle = rangeDown;
        }
        if (angle > rangeUp) {
            angle = rangeUp;
        }
        servo.setEasingType(EASE_CUBIC_IN_OUT);
        servo.setEaseTo((float)angle);
        int distance = abs(angle - prev);
        prev = angle;

        return distance * SERVO_SPEED;
    }
};

class Leg {
  public:
    int id;
    Joint *hip;
    Joint *knee;
    Joint *ankle;
    int planeAngle = 0; //from horizontal
    pos defaultPos =  pos(0, 6.4, -5.9); // centre of gcircle
    int previous = 0;
    int radius = 8.9;


    Leg(int id, int info[3][4])
    : id(id) {
        hip = new Joint(id*HIP, info[0]);
        knee = new Joint(id*KNEE, info[1]);
        ankle = new Joint(id*ANKLE, info[2]);
        
        if (abs(id) == FRONT) {
            planeAngle = 60;
        } else if (abs(id) == BACK) {
            planeAngle = -60;
        }
    }

    int goTo(pos dest) {
        // if (id < 0) {
        //     dest = pos(-dest.x, -dest.y, dest.z);
        // }
        pos angles = InvK::getAngles(dest);
        angles.x = -angles.x;

        int ret = hip->write(angles.x);
        int ret1 = knee->write(angles.y);
        int ret2 = ankle->write(angles.z);

        ret = max(ret1, ret);
        ret = max(ret, ret2);

        return ret;
    }

    int gCircleTo(int angle, int z = 0) {
        int multiplier = 1;
        if (id < 0) {
            angle -=180;
            multiplier = -1;
        }
        angle += multiplier*planeAngle;
        while (angle <= -180) angle += 360;
        while (angle > 180) angle -= 360;
        previous = angle;
        return goTo(defaultPos + pos(G_CIRCLE_RADIUS*cos((PI/180)*angle),G_CIRCLE_RADIUS*sin((PI/180)*angle),z));
    }

    int gCircleTurn(int angle, int z = 0) {
        angle += planeAngle;
        while (angle <= -180) angle += 360;
        while (angle > 180) angle -= 360;
        previous = angle;
        return goTo(defaultPos + pos(G_CIRCLE_RADIUS*cos((PI/180)*angle),G_CIRCLE_RADIUS*sin((PI/180)*angle),z));
    }

    int goToRel(pos dest) {
        return goTo(defaultPos + dest);
    }

    int gCircleRel(int angle) {
        return gCircleTo(angle + previous);
    }

    // ellipseTo takes in time and direction and returns the position on the ellipse
    void ellipseTo(double time, int angle) {
        int multiplier = 1;
        if (id < 0) {
            angle -=180;
            multiplier = -1;
        }
        angle += multiplier*planeAngle;
        while (angle <= -180) angle += 360;
        while (angle > 180) angle -= 360;

        double t = time * PI/500 * ELLIPSE_SPEED;
        // calculat z coordinate and x + y coordinates using 2d parametric equation
        double z = SR_ELLIPSE * cos(t);
        double xAndY = LR_ELLIPSE * sin(t);
        // then use x + y coordinate and angle to calculate x and y coordinates
        double x = xAndY * cos(angle);
        double y = xAndY * -sin(angle);
        // return pos(x,y,z)
        pos coordinate = pos(x, y, z);
        // then call goToRel
        goToRel(coordinate);
    }

    int stance() {
        return goTo(defaultPos);
    }
};

class Body {
  public:
    Leg *legs[7]; //one spot lost in translation :()
    
    Body() {} // stupid platformio being baka

    // LEFT FRONT MID BACK THEN RIGHT
    Body(int info[6][3][4]) {
        legs[ind(FRONT*LEFT)]   =   new Leg(FRONT*LEFT, info[0]);
        legs[ind(MID*LEFT)]     =   new Leg(MID*LEFT, info[1]);
        legs[ind(BACK*LEFT)]    =   new Leg(BACK*LEFT, info[2]);
        legs[ind(FRONT*RIGHT)]  =   new Leg(FRONT*RIGHT, info[5]);
        legs[ind(MID*RIGHT)]    =   new Leg(MID*RIGHT, info[4]);
        legs[ind(BACK*RIGHT)]   =   new Leg(BACK*RIGHT, info[3]);
    }

    static int ind(int id) {
        return id += RIGHT*BACK;
    }

    Leg *leg(int id) {
      return legs[ind(id)];
    }

    // DOES NOT WORK :(
    void swingAround(int rotations = 5) {
        int increment = 5;
        double heights[6] = {1, -1, 0.5, -0.5, 0.25, -0.25};
        for (int i = 0, j = 0; i < 360*rotations ; i += increment, j++) {
            int wait = max(leg(FRONT*LEFT)->gCircleTo(0 + i, heights[j%6]), leg(BACK*RIGHT)->gCircleTo(0 + i, heights[(j+1)%6]));
            wait = max(leg(MID*LEFT)->gCircleTo(20 + i, heights[(j+2)%6]), leg(MID*RIGHT)->gCircleTo(20 + i, heights[(j+3)%6]));
            wait = max(leg(BACK*LEFT)->gCircleTo(40 + i , heights[(j+4)%6]), leg(FRONT*RIGHT)->gCircleTo(40 + i , heights[(j+5)%6]));
            updateAndWaitForAllServosToStop();
        }
    }

    void tripodgait(int angle = 0) {
        Leg *tripods[2][3] =   {{leg(FRONT*RIGHT), leg(BACK*RIGHT), leg(MID*LEFT)},
                                {leg(FRONT*LEFT), leg(BACK*LEFT), leg(MID*RIGHT)}
                            };
        int lead = 0;
        double start = millis();
        double t = millis();

        while(t > 0) {
            double time = t - start;
            for (int j = 0; j < 3; j++) {
                tripods[lead][j]->ellipseTo(time, angle);
                tripods[1-lead][j]->ellipseTo(time + 500/ELLIPSE_SPEED, angle);
            }
            synchronizeAllServosStartAndWaitForAllServosToStop();
            t = millis();
        }
    }

    void tripodturn(boolean left) {
        double angle = 180;
        if (left) {
            angle = 0;
        }
        Leg *tripods[2][3] =   {{leg(FRONT*RIGHT), leg(BACK*RIGHT), leg(MID*LEFT)},
                                {leg(FRONT*LEFT), leg(BACK*LEFT), leg(MID*RIGHT)}
                            };
        int lead = 0;
        for (int i = 0; i < 2*10; i++) {
            int wait = 0;
            int wait1 = 0;
            for (int j = 0; j < 3; j++) {
                wait = tripods[lead][j]->goToRel(pos(0, 0, 5));
                wait1 = tripods[1-lead][j]->goToRel(pos(0, 0, 1));
                wait = max(wait, wait1);
            }
            synchronizeAllServosStartAndWaitForAllServosToStop();
            wait = 0;
            wait1 = 0;
            for (int j = 0; j < 3; j++) {
                wait = tripods[lead][j]->gCircleTurn(angle);
                wait1 = tripods[1-lead][j]->gCircleTurn(angle - 180);
                wait = max(wait, wait1);
            }
            synchronizeAllServosStartAndWaitForAllServosToStop();
            lead = 1 - lead;

        }
    }

    void wavegait(int angle = 0) {
        Leg *wave[6] = {leg(FRONT*RIGHT), leg(MID*RIGHT), leg(BACK*RIGHT), 
                        leg(BACK*LEFT), leg(MID*LEFT), leg(FRONT*LEFT)}; 
        for (int i = 0; i < 6; i++) {
            delay(wave[i]->goToRel(pos(0,0,5)));
            int wait = wave[i]->gCircleTo(180 - angle);
            int wait1 = 0;
            for (int j = 0; j < 6; j++) {
                if (wave[j] != wave[i]) {
                    int current = wave[j]->gCircleRel(-180/6);
                    wait1 = max(current, wait1);
                }
            }
            wait = max(wait, wait1);
            delay(wait);
        }
    }

    void stance() {
        int ret = leg(FRONT*RIGHT)->stance();
        int ret1 = leg(MID*RIGHT)->stance();
        int ret2 = leg(BACK*RIGHT)->stance();
        int ret3 = leg(FRONT*LEFT)->stance();
        int ret4 = leg(MID*LEFT)->stance();
        int ret5 = leg(BACK*LEFT)->stance();
        ret = max(ret, ret1);
        ret = max(ret, ret2);
        ret = max(ret, ret3);
        ret = max(ret, ret4);
        ret = max(ret, ret5);
        updateAndWaitForAllServosToStop();
        // delay(ret);
    }

};