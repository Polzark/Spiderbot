#include <Arduino.h>

#define G_CIRCLE_RADIUS 3.5 // in cm
#define BOT_RADIUS 6.4 // DEFINITELY NOT IN CM (despite definitely being in cm??)

struct pos
{
    double x = 0;
    double y = 0;
    double z = 0;

    pos(double x, double y, double z) : x(x), y(y), z(z) {}

    pos& operator=(const pos& a) {
        x = a.x;
        y = a.y;
        z = a.z;
        return *this;
    }

    pos operator+(const pos& a) const {
        return pos(a.x+x, a.y+y, a.z+z);
    }

    pos operator-(const pos& a) const {
        return pos(x-a.x, y-a.y, z-a.z);
    }

    bool operator==(const pos& a) const {
        return (x == a.x && y == a.y && z == a.z);
    }
};

class InvK {
    public:
    static constexpr double link1 = 6.4;
    static constexpr double link2 = 5.9;

    static pos getAngles(pos dest) {

        double x = dest.x;
        double y = dest.y;
        double z = dest.z;
        double r = sqrt(x*x + y*y + z*z);
        double theta1, theta2, theta3, eha, eah;

        double temp1 = (link1*link1 + link2*link2 - r*r)/(2*link1*link2);
        if (abs(temp1) > 1) {
            return pos(0, 0, 0);
        }

        eah = acos(temp1);

        double temp2 = link2*sin(eah)/r;

        if (abs(temp2) > 1) {
            return pos(0, 0, 0);
        }

        eha = asin(temp2);
        

        if (y == 0) {
            if (x > 0) {
                theta1 = 90;
            } else if (x < 0) {
                theta1 = -90;
            } else {
                theta1 = 0;
            }
        } else {
            theta1 = (180/PI)*atan(x/y);
        }

        if (r == abs(z)) {
            theta2 = (180/PI)*(PI/2 + eha);
        } else {
            theta2 = (180/PI) * (atan(z/(sqrt(x*x + y*y))) + eha);
        }

        theta3 = (180/PI)*eah - 180;
        
        return pos(theta1, theta2, theta3);
    }
};