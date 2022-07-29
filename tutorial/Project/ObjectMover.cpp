#include "ObjectMover.h"

#include <utility>

ObjectMoverSplit::ObjectMoverSplit(std::shared_ptr<ObjectMover> firstMover, std::string name):
    name(name) {
    if(firstMover->getStartTime() < 0)
        throw std::invalid_argument("Input mover must start at t>0!");
    movers.push_back(firstMover);
}

ObjectMoverSplit::ObjectMoverSplit() {
    auto firstMover = std::make_shared<ObjectMoverConstant>(Eigen::Vector3f(0,0,0), 0, 1);
    movers.push_back(firstMover);
}

ObjectMoverSplit::ObjectMoverSplit(std::vector<std::shared_ptr<ObjectMover>> movers, std::string name):
    movers(std::move(movers)), name(name) {}

Eigen::Vector3f ObjectMoverSplit::getPosition(float time) {
   for(const std::shared_ptr<ObjectMover>& mover : movers) {
       if(mover->getStartTime() <= time && time < mover->getEndTime())
           return mover->getPosition(time);
   }
    return movers.back()->getPosition(movers.back()->getEndTime());
}

float ObjectMoverSplit::getStartTime() {
    return movers.front()->getStartTime();
}

float ObjectMoverSplit::getEndTime() {
    return movers.back()->getEndTime();
}

MoverType ObjectMoverSplit::getTag() {
    return SPLIT;
}

void ObjectMoverSplit::shift(Eigen::Vector3f shiftValue) {
    for(const std::shared_ptr<ObjectMover>& mover : movers) {
        mover->shift(shiftValue);
    }
}

void ObjectMoverSplit::addMover(std::shared_ptr<ObjectMover> mover) {
    if(mover->getStartTime() != movers.back()->getEndTime()) {
        throw std::invalid_argument("Input mover must start when current mover ends!");
    }
    movers.push_back(mover);

}

std::shared_ptr<ObjectMover> ObjectMoverSplit::clone() {
    std::vector<std::shared_ptr<ObjectMover>> newMovers;
    for(auto &mover : movers)
       newMovers.push_back(mover->clone());

    return std::make_shared<ObjectMoverSplit>(movers, name);
}

std::shared_ptr<ObjectMoverSplit> ObjectMoverSplit::cloneAndCast() {
    return std::static_pointer_cast<ObjectMoverSplit>(clone());
}


ObjectMoverConstant::ObjectMoverConstant(Eigen::Vector3f pos, float startTime, float duration) {
    if(duration < 0){
        throw std::invalid_argument("Input mover duration must be > 0!");
    }
    this->position = std::move(pos);
    this->startTime = startTime;
    this->endTime = startTime + duration;
}

Eigen::Vector3f ObjectMoverConstant::getPosition(float time) {
    return position;
}

float ObjectMoverConstant::getStartTime() {
    return startTime;
}

float ObjectMoverConstant::getEndTime() {
    return endTime;
}

MoverType ObjectMoverConstant::getTag() {
    return CONSTANT;
}

void ObjectMoverConstant::shift(Eigen::Vector3f shiftValue) {
    position += shiftValue;
}

std::shared_ptr<ObjectMover> ObjectMoverConstant::clone() {
    return std::make_shared<ObjectMoverConstant>(position, startTime, endTime-startTime);
}

ObjectMoverBezier::ObjectMoverBezier(const std::vector<Eigen::Vector3f>& points, float startTime, float duration) {
    if(duration < 0){
        throw std::invalid_argument("Input mover duration must be > 0!");
    }
    if(points.empty()){
        throw std::invalid_argument("One or more points are required for bezier");
    }
    this->points = points;
    this->startTime = startTime;
    this->endTime = startTime + duration;
}
int BinomialCoefficient(const int n, const int k) {
    if(k==0)
        return 1;
    std::vector<int> aSolutions(k);
    aSolutions[0] = n - k + 1;

    for (int i = 1; i < k; ++i) {
        aSolutions[i] = aSolutions[i - 1] * (n - k + 1 + i) / (i + 1);
    }

    return aSolutions[k - 1];
}
Eigen::Vector3f ObjectMoverBezier::getPosition(float time) {
    if(time < startTime || time > endTime)
        throw std::invalid_argument("Time out of bounds for curve");
    float normalizedTime = (time - startTime) / (endTime - startTime);
    size_t n = points.size();
    Eigen::Vector3f pos = Eigen::Vector3f(0,0,0);
    for(std::size_t i=0; i<n; i++) {
        pos += BinomialCoefficient(n, i) * pow((1-normalizedTime), n-i) * pow(normalizedTime, i) * points[i];
    }
    return pos;
}

float ObjectMoverBezier::getStartTime() {
    return startTime;
}

float ObjectMoverBezier::getEndTime() {
    return endTime;
}

MoverType ObjectMoverBezier::getTag() {
    return BEZIER;
}

void ObjectMoverBezier::shift(Eigen::Vector3f shiftValue) {
    for(Eigen::Vector3f &point : points)
        point += shiftValue;
}

std::shared_ptr<ObjectMover> ObjectMoverBezier::clone() {
    std::vector<Eigen::Vector3f> pointsCopy = points;
    return std::make_shared<ObjectMoverBezier>(pointsCopy, startTime, endTime-startTime);
}

bool ObjectMover::isDrawnAt(float time) {
    return getStartTime() <= time && time <= getEndTime();
}
