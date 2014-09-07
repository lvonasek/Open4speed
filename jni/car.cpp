//----------------------------------------------------------------------------------------
/**
 * \file       car.cpp
 * \author     Vonasek Lubos
 * \date       2014/01/05
 * \brief      Class for rendering car and updating navigation on track. It also loads
 *             informations about physical powers of current car.
*/
//----------------------------------------------------------------------------------------

#include "utils/io.h"
#include "utils/math.h"
#include "utils/switch.h"
#include "car.h"
#include "common.h"

#define PERSPECTIVE_MIN 90
#define PERSPECTIVE_MAX 150
#define PERSPECTIVE_SPEED_DEPENDENCY 0.2
#define PERSPECTIVE_SPEED_FOLLOW 2
#define SPEED_ASPECT 50
#define SOUND_CRASH_MINIMAL_SPEED 5
#define SOUND_CRASH_ON_SPEED_CHANGE 0.7
#define SOUND_ENGINE_FREQ_ASPECT 15
#define SOUND_MAXIMAL_DISTANCE 25

/**
 * @brief car is constructor which loads car model
 * @param i is car control device(or program)
 * @param e is curve which car may take on
 * @param filename is path to file to load
 * @param automatic is true for automatic transmision
 */
car::car(input *i, std::vector<edge> *e, std::string filename) {

    /// get car atributes
    std::vector<std::string> atributes = getList("", filename);

    /// set default values
    n2o = 150;
    onRoof = 0;
    extraSound = 0;
    control = i;
    edges = *e;
    prevEffect = 0;
    rot = 0;
    speed = 0;
    tempRot = 0;
    view = 60;
    reverse = false;
    resetAllowed = false;
    resetRequested = false;

    /// create matrices
    transform = new matrix[5];
    for (int i = 0; i < 5; i++) {
        transform[i].temp = new float[16];
        for (int j = 0; j < 16; j++)
            transform[i].temp[j] = 0;
        transform[i].temp[0] = 1;
        transform[i].temp[5] = 1;
        transform[i].temp[10] = 1;
        transform[i].temp[15] = 1;

        transform[i].value = new float[16];
        for (int j = 0; j < 16; j++)
            transform[i].value[j] = 0;
        transform[i].value[0] = 1;
        transform[i].value[5] = 1;
        transform[i].value[10] = 1;
        transform[i].value[15] = 1;
    }

    /// apply index of car
    index = allCar.size() + 1;
    if (control != 0)
        control->index = index - 1;

    /// load models
    skin = getModel(getConfigStr("skin_model", atributes), true);
    wheel = getModel(getConfigStr("wheel_model", atributes), true);

    /// set car wheels position
    wheelX = getConfig("wheel_x", atributes);
    wheelY = getConfig("wheel_y", atributes);
    wheelZ1 = getConfig("wheel_back", atributes);
    wheelZ2 = getConfig("wheel_front", atributes);
    brakePower = getConfig("brake_power", atributes);
    steering = getConfig("steering", atributes);

    /// set gears
    currentGear = 1;
    int gearCount = getConfig("gear_count", atributes);
    for (int i = 0; i <= gearCount; i++) {
        gear g;
        g.min = getConfig(getTag(i,"gear%d_min"), atributes);
        g.max = getConfig(getTag(i,"gear%d_max"), atributes);
        gears.push_back(g);
    }
    gearLow = getConfig("gear_low", atributes);
    gearHigh = getConfig("gear_high", atributes);
    gearUp = getConfig("gear_up", atributes);
    gearDown = getConfig("gear_down", atributes);
    mass = getConfig("mass", atributes);
    power = getConfig("power", atributes);
    lowAspect = getConfig("low_aspect", atributes);
    acceleration = 0;
}

/**
 * @brief car destructor
 */
car::~car() {
    delete control;
    for (int i = 0; i < 5; i++) {
        delete[] transform[i].temp;
        delete[] transform[i].value;
    }
    delete[] transform;
}

/**
 * @brief getView gets perspective view of car
 * @return view perspective by car speed
 */
float car::getView() {
    view = (view + (PERSPECTIVE_MIN + speed * PERSPECTIVE_SPEED_DEPENDENCY)) / PERSPECTIVE_SPEED_FOLLOW;
    if (view < PERSPECTIVE_MIN)
        view = PERSPECTIVE_MIN;
    if (view > PERSPECTIVE_MAX)
        view = PERSPECTIVE_MAX;
    return view;
}

/**
 * @brief setStart sets start position of car
 * @param e is curve where car starts on
 */
void car::setStart(edge e) {

    /// error check
    if ((e.a.x == e.b.x) && (e.a.z == e.b.z)) {
        return;
    }

    /// set edges
    currentEdge = e;
    currentGoalEdge = e;

    /// set transformation
    resetAllowed = false;
    reverse = false;
    rot = (float)angle(currentEdge.a, currentEdge.b) * 180 / 3.14 - 180;
    oldPos.x = pos.x = currentEdge.a.x;
    oldPos.y = pos.y = currentEdge.a.y;
    oldPos.z = pos.z = currentEdge.a.z;

    /// count distance from finish
    int ltg = lapsToGo;
    if (ltg < 0) {
        toFinish = 0;
        return;
    }
    edge cge = currentGoalEdge;
    toFinish = glm::length(cge.a - cge.b);
    while (true) {
        toFinish += glm::length(cge.a - cge.b);
        std::vector<int> nEdges = nextEdge(&edges, cge);
        if (nEdges.size() > 0) {
            cge = edges[nEdges[0]];
            if ((ltg > 0) & (nEdges[0] == finishEdge)) {
                ltg--;
            } else if ((ltg == 0) & (nEdges[0] == finishEdge)) {
                break;
            }
        } else {
            break;
        }
    }
}


/**
 * @brief update updates car wheels state(rotation and steering)
 */
void car::update() {

    /// count speed
    lspeed = speed;
    speed = sqrt(sqr(fabsf(oldPos.x - pos.x)) + sqr(fabsf(oldPos.z - pos.z))) * SPEED_ASPECT;

    /// update current edge for counting laps
    edge cge = currentGoalEdge;
    if (distance(pos, currentGoalEdge.b) < 75) {
        std::vector<int> nEdges = nextEdge(&edges, currentGoalEdge);
        if (nEdges.size() > 0) {
            if (isSame(edges[nEdges[0]], edges[finishEdge])) {
                if (distance(pos, currentGoalEdge.b) < 25) {
                    toFinish -= glm::length(cge.a - cge.b);
                    currentEdgeIndex = nEdges[0];
                    currentGoalEdge = edges[nEdges[0]];
                    if ((lapsToGo >= 0) & (isSame(edges[nEdges[0]], edges[finishEdge]))) {
                        lapsToGo--;
                    }
                }
            } else {
                toFinish -= glm::length(cge.a - cge.b);
                currentEdgeIndex = nEdges[0];
                currentGoalEdge = edges[nEdges[0]];
                if ((lapsToGo >= 0) & (isSame(edges[nEdges[0]], edges[finishEdge]))) {
                    lapsToGo--;
                }
            }
        }
    }

    /// store last position
    oldPos.x = pos.x;
    oldPos.y = pos.y;
    oldPos.z = pos.z;

    /// reverse gear auto change
    if (reverse & (currentGear == 1))
        currentGear = 0;
    else if (!reverse & (currentGear == 0)) {
        currentGear = 1;
    }

    /// count acceleration
    float speedDiff = gears[currentGear].max - gears[currentGear].min;
    float speedAspect = (speed - gears[currentGear].min) / speedDiff;
    if (speedAspect > 1)
        speedAspect = 1;
    if (speedAspect < 0)
        speedAspect = 0;
    acceleration = lowAspect * power + (1 - lowAspect) * power * speedAspect;
}

/**
 * @brief updates car transformation
 */
void car::updateMatrices() {
    /// update matrices
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 16; j++) {
            transform[i].value[j] = transform[i].temp[j];
        }
    }
    rot = tempRot;


    /// reverse camera
    if (rot > 360)
        rot -= 360;
    if (reverse & (speed > 3))
        rot -= 180;
    if (rot < 0)
        rot += 360;
}

/**
 * @brief updates car sound
 */
void car::updateSound() {


    /// play crash
    bool brake = false;
    if (!reverse & (control->getBrake() < 1))
        brake = true;
    if (reverse & (control->getGas() < 1))
        brake = true;
    if ((lspeed * SOUND_CRASH_ON_SPEED_CHANGE > speed) & (speed > SOUND_CRASH_MINIMAL_SPEED) & brake) {
        crash->play(index - 1);
    }

    /// set nitro sound
    if ((control->getNitro() > 0) && (n2o > 1)) {
        noise->play(index - 1);
        noise->setFrequency(index - 1, speed * 200);
    } else {
        noise->stop(index - 1);
    }

    /// count engine frequency
    float speedPlus = speed - gears[currentGear].min;
    if (speedPlus < 0)
        speedPlus = 0;
    float speedDiff = gears[currentGear].max - gears[currentGear].min;
    float engineFreq = gearLow + (gearHigh - gearLow) * speedPlus / speedDiff;

    /// automatic changing gears
    unsigned int lastGear = currentGear;
    if (currentGear >= 1) {
        if ((currentGear > 1) & (engineFreq < gearDown))
            currentGear--;
        if (currentGear < gears.size() - 1) {
            if (engineFreq > gearUp)
                currentGear++;
        }
    }

    /// update extra sound volume
    if (speed - lspeed > 0)
        extraSound += 0.01f;
    else
        extraSound -= 0.01f;
    if (lastGear > currentGear)
        extraSound = 1;
    if (extraSound < 0.05)
        extraSound = 0.05;
    if (extraSound > 0.25)
        extraSound = 0.25;

    /// set engine sound
    if (engineFreq >= gearLow) {
        engine->play(index - 1);
        engine->setFrequency(index - 1, engineFreq * SOUND_ENGINE_FREQ_ASPECT);
        enginePlus->play(index - 1);
        enginePlus->setFrequency(index - 1, engineFreq * SOUND_ENGINE_FREQ_ASPECT);
    } else if (engineFreq < gearLow) {
        engine->setFrequency(index - 1,  gearLow * SOUND_ENGINE_FREQ_ASPECT);
        enginePlus->setFrequency(index - 1,  gearLow * SOUND_ENGINE_FREQ_ASPECT);
    }

    /// set sound distance
    float dist = (1 - distance(allCar[cameraCar]->pos, pos) / SOUND_MAXIMAL_DISTANCE);
    if (dist < 0)
        dist = 0;
    dist *= dist;
    crash->setVolume(index - 1, dist);
    engine->setVolume(index - 1, dist * 8.0 * (0.25f - extraSound));
    enginePlus->setVolume(index - 1, dist * extraSound);
    noise->setVolume(index - 1, dist);
}
