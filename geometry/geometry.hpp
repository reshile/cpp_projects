#include <iostream>

class Vector;

class IShape;

class Point;

class Segment;

class Vector {
 private:
  int64_t xcord_;
  int64_t ycord_;

 public:
  Vector();

  Vector(int64_t, int64_t);

  Vector(const Vector&);

  Vector(const Point&);

  Vector& operator=(const Vector&);

  int64_t operator*(const Vector&) const;

  Vector& operator+=(const Vector&);

  Vector& operator-=(const Vector&);

  Vector operator-() const;

  Vector& operator*=(int64_t);

  int64_t GetX() const;

  int64_t GetY() const;

  ~Vector() {}
};

int64_t operator^(const Vector&, const Vector&);

Vector operator*(Vector vect, const int64_t& num);

Vector operator*(const int64_t& num, Vector vect);

Vector operator+(Vector left, const Vector& right);

Vector operator-(Vector left, const Vector& right);

bool operator==(const Vector& right, const Vector& left);

bool Crossfunct(const Segment& segment, const Point& point);

class IShape {
 public:
  virtual void Move(const Vector&) = 0;

  virtual bool ContainsPoint(const Point&) const = 0;

  virtual bool CrossSegment(const Segment&) const = 0;

  virtual IShape* Clone() const = 0;

  virtual ~IShape() = 0;
};

class Point : public IShape {
 private:
  int64_t xcord_;
  int64_t ycord_;

 public:
  Point(int64_t xcord, int64_t ycord) : xcord_(xcord), ycord_(ycord) {}

  Point(const Point& right) : xcord_(right.xcord_), ycord_(right.ycord_) {}

  Point(const Vector& vector) : xcord_(vector.GetX()), ycord_(vector.GetY()) {}

  Point& operator=(const Point& right) {
    xcord_ = right.xcord_;
    ycord_ = right.ycord_;
    return *this;
  }

  Point& operator+=(const Vector& vector) {
    xcord_ += vector.GetX();
    ycord_ += vector.GetY();
    return *this;
  }

  Point& operator-=(const Point& right) {
    xcord_ -= right.xcord_;
    ycord_ -= right.ycord_;
    return *this;
  }

  int64_t GetX() const { return xcord_; }

  int64_t GetY() const { return ycord_; }

  void Move(const Vector& vector) override {
    xcord_ += vector.GetX();
    ycord_ += vector.GetY();
  }

  bool ContainsPoint(const Point& point) const override {
    return (xcord_ == point.xcord_ && ycord_ == point.ycord_);
  }

  bool CrossSegment(const Segment& segment) const override {
    return Crossfunct(segment, *this);
  }

  IShape* Clone() const override {
    Point* clone = new Point(xcord_, ycord_);
    return clone;
  };

  ~Point() {}
};

Point operator-(Point left, const Point& right);

class Segment : public IShape {
 private:
  Point begin_;
  Point end_;

 public:
  Segment(Point begin, Point end) : begin_(begin), end_(end) {}

  Point GetA() const { return begin_; }

  Point GetB() const { return end_; }

  void Move(const Vector& vector) override {
    begin_ += vector;
    end_ += vector;
  }

  bool ContainsPoint(const Point& point) const override {
    Vector ab_vect((end_ - begin_).GetX(), (end_ - begin_).GetY());
    Vector ac_vect((point - begin_).GetX(), (point - begin_).GetY());
    Vector bc_vect((point - end_).GetX(), (point - end_).GetY());
    Vector ba_vect = -ab_vect;
    if (ab_vect == ba_vect) {
      return point.ContainsPoint(begin_);
    }
    return (ac_vect ^ ab_vect) == 0 && (ac_vect * ab_vect) >= 0 &&
           (bc_vect * ba_vect) >= 0;
  }

  bool CrossSegment(const Segment& segment) const override {
    Point a_p = begin_;
    Point b_p = end_;
    Point c_p = segment.begin_;
    Point d_p = segment.end_;
    Vector ab_vect((b_p - a_p).GetX(), (b_p - a_p).GetY());
    Vector cd_vect((d_p - c_p).GetX(), (d_p - c_p).GetY());
    Vector ac_vect((c_p - a_p).GetX(), (c_p - a_p).GetY());
    Vector ad_vect((d_p - a_p).GetX(), (d_p - a_p).GetY());
    Vector ca_vect = -ac_vect;
    Vector cb_vect((b_p - c_p).GetX(), (b_p - c_p).GetY());
    if ((ab_vect ^ cd_vect) != 0) {
      int64_t expr1 = (ab_vect ^ ac_vect) * (ab_vect ^ ad_vect);
      int64_t expr2 = (cd_vect ^ ca_vect) * (cd_vect ^ cb_vect);
      return (expr1 <= 0 && expr2 <= 0);
    }
    if (this->ContainsPoint(c_p) || this->ContainsPoint(d_p)) {
      return true;
    }
    return (segment.ContainsPoint(a_p) || segment.ContainsPoint(b_p));
  }

  IShape* Clone() const override {
    Segment* clone = new Segment(begin_, end_);
    return clone;
  }

  ~Segment() {}
};

class Line : public IShape {
 private:
  Point first_;
  Point second_;

 public:
  Line(Point first, Point second) : first_(first), second_(second) {}

  int64_t GetA() const { return (second_ - first_).GetY(); }

  int64_t GetB() const { return -(second_ - first_).GetX(); }

  int64_t GetC() const {
    int64_t result = (first_.GetY()) * (-GetB()) - (first_.GetX()) * GetA();
    return result;
  }

  void Move(const Vector& vector) override {
    first_ += vector;
    second_ += vector;
  }

  bool ContainsPoint(const Point& point) const override {
    Vector ab_vect((second_ - first_).GetX(), (second_ - first_).GetY());
    Vector ac_vect((point - first_).GetX(), (point - first_).GetY());
    return ((ac_vect ^ ab_vect) == 0);
  }

  bool CrossSegment(const Segment& segment) const override {
    Point a_p = first_;
    Point b_p = second_;
    Point c_p = segment.GetA();
    Point d_p = segment.GetB();
    Vector ab_vect((b_p - a_p).GetX(), (b_p - a_p).GetY());
    Vector ac_vect((c_p - a_p).GetX(), (c_p - a_p).GetY());
    Vector ad_vect((d_p - a_p).GetX(), (d_p - a_p).GetY());
    return ((ab_vect ^ ac_vect) * (ab_vect ^ ad_vect) <= 0);
  }

  IShape* Clone() const override {
    Line* clone = new Line(first_, second_);
    return clone;
  }
};

class Ray : public IShape {
 private:
  Point first_;
  Point second_;

 public:
  Ray(Point first, Point second) : first_(first), second_(second) {}

  Point GetA() const { return first_; }

  Vector GetVector() const {
    Point forvect = second_ - first_;
    Vector direct(forvect.GetX(), forvect.GetY());
    return direct;
  };

  void Move(const Vector& vector) override {
    first_ += vector;
    second_ += vector;
  };

  bool ContainsPoint(const Point& point) const override {
    Line tmp(first_, second_);
    bool online = tmp.ContainsPoint(point);
    Vector pvect((point - first_).GetX(), (point - first_).GetY());
    Vector rvect = this->GetVector();
    return (online && (pvect * rvect >= 0));
  }

  bool CrossSegment(const Segment& segment) const override {
    Point a_p = segment.GetA();
    Point b_p = segment.GetB();
    Line line(first_, second_);
    if (this->ContainsPoint(a_p) || this->ContainsPoint(b_p)) {
      return true;
    }
    // Checking if segment crosses line at all.
    if (!(line.CrossSegment(segment))) {
      return false;
    }
    Line seg_line(a_p, b_p);
    Vector seg_normal(seg_line.GetA(), seg_line.GetB());
    Vector seg_start(a_p.GetX(), a_p.GetY());
    Vector ray_start(first_.GetX(), first_.GetY());
    Vector ray_vector = this->GetVector();
    double seg_val = (seg_start * seg_normal);
    double ray_start_to_norm = (ray_start * seg_normal);
    double ray_to_norm = (ray_vector * seg_normal);
    double intersection = (seg_val - ray_start_to_norm) / ray_to_norm;
    return (intersection >= 0);
  }

  IShape* Clone() const override {
    Ray* clone = new Ray(first_, second_);
    return clone;
  }

  ~Ray() {}
};

class Circle : public IShape {
 private:
  Point centre_;
  size_t radius_;

 public:
  Circle(Point centre, size_t radius) : centre_(centre), radius_(radius) {}

  Point GetCentre() const { return centre_; }

  size_t GetRadius() const { return radius_; }

  void Move(const Vector& vector) override { centre_ += vector; }

  bool ContainsPoint(const Point& point) const override {
    Point diff = point - centre_;
    int64_t expression =
        (diff.GetX() * diff.GetX()) + (diff.GetY() * diff.GetY());
    return (expression <= static_cast<int64_t>(radius_ * radius_));
  }

  bool PointInCircle(const Point& point) const {
    Point diff = point - centre_;
    int64_t expression =
        (diff.GetX() * diff.GetX()) + (diff.GetY() * diff.GetY());
    return (expression < static_cast<int64_t>(radius_ * radius_));
  }

  bool CrossSegment(const Segment& segment) const override {
    Point a_p = segment.GetA();
    Point b_p = segment.GetB();
    if (this->PointInCircle(a_p) && this->PointInCircle(b_p)) {
      return false;
    }
    Vector ab_vect((b_p - a_p).GetX(), (b_p - a_p).GetY());
    Vector ac_vect((centre_ - a_p).GetX(), (centre_ - a_p).GetY());
    Vector bc_vect((centre_ - b_p).GetX(), (centre_ - b_p).GetY());
    int64_t ab_len =
        ab_vect.GetX() * ab_vect.GetX() + ab_vect.GetY() * ab_vect.GetY();
    if ((ac_vect ^ bc_vect) * (ac_vect ^ bc_vect) >
        static_cast<int64_t>(radius_ * radius_) * ab_len) {
      return false;
    }
    if (this->ContainsPoint(a_p) ^ this->ContainsPoint(b_p)) {
      return true;
    }
    Vector ba_vect = -ab_vect;
    return ((ba_vect * bc_vect) > 0 && (ab_vect * ac_vect) > 0);
  }

  IShape* Clone() const override {
    Circle* clone = new Circle(centre_, radius_);
    return clone;
  }
};

