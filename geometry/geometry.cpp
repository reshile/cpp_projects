#include "geometry.hpp"

#include <iostream>

Vector::Vector() : xcord_(0), ycord_(0) {}

Vector::Vector(int64_t xcord, int64_t ycord) : xcord_(xcord), ycord_(ycord) {}

Vector::Vector(const Point& point)
    : xcord_(point.GetX()), ycord_(point.GetY()) {}

Vector::Vector(const Vector& right)
    : xcord_(right.xcord_), ycord_(right.ycord_) {}

Vector& Vector::operator=(const Vector& right) {
  xcord_ = right.xcord_;
  ycord_ = right.ycord_;
  return *this;
}

int64_t Vector::operator*(const Vector& right) const {
  int64_t scalar = (xcord_ * right.xcord_) + (ycord_ * right.ycord_);
  return scalar;
}

Vector& Vector::operator+=(const Vector& right) {
  xcord_ += right.xcord_;
  ycord_ += right.ycord_;
  return *this;
}

Vector& Vector::operator-=(const Vector& right) {
  xcord_ -= right.xcord_;
  ycord_ -= right.ycord_;
  return *this;
}

Vector Vector::operator-() const {
  Vector copy = *this;
  copy.xcord_ = -xcord_;
  copy.ycord_ = -ycord_;
  return copy;
}

Vector& Vector::operator*=(int64_t num) {
  xcord_ *= num;
  ycord_ *= num;
  return *this;
}

Vector operator*(Vector vect, const int64_t& num) {
  vect *= num;
  return vect;
}

Vector operator*(const int64_t& num, Vector vect) {
  vect *= num;
  return vect;
}

int64_t Vector::GetX() const { return xcord_; }

int64_t Vector::GetY() const { return ycord_; }

Vector operator+(Vector left, const Vector& right) {
  left += right;
  return left;
}

Vector operator-(Vector left, const Vector& right) {
  left -= right;
  return left;
}

int64_t operator^(const Vector& left, const Vector& right) {
  int64_t result = left.GetX() * right.GetY() - left.GetY() * right.GetX();
  return result;
}

bool operator==(const Vector& right, const Vector& left) {
  return (right.GetX() == left.GetX() && right.GetY() == left.GetY());
}

IShape::~IShape() {}

Point operator-(Point left, const Point& right) {
  left -= right;
  return left;
}

bool Crossfunct(const Segment& segment, const Point& point) {
  return segment.ContainsPoint(point);
}
