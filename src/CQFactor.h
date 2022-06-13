#ifndef CQFactor_H
#define CQFactor_H

#include <QWidget>
#include <vector>
#include <cmath>

class QSpinBox;
class QTimer;

namespace CQFactor {

class App;

class Window : public QWidget {
  Q_OBJECT

 public:
  Window(QWidget *parent=0);

  void setFactor(int i);

  QSize sizeHint() const { return QSize(800, 800); }

 signals:
  void factorEntered(int i);

 private slots:
  void factorSlot();
  void debugSlot(int);

 private:
  App*      app_  { nullptr };
  QSpinBox* edit_ { nullptr };
};

//------

class Circle;

using Circles = std::vector<Circle *>;

//----

struct DrawCircle {
  QRectF rect;
  QColor pen;
  QColor brush;
  QRectF oldRect;
  QColor oldPen;
  QColor oldBrush;

  DrawCircle() = default;

  DrawCircle(const QRectF &rect, const QColor &pen, const QColor &brush) :
   rect(rect), pen(pen), brush(brush) {
  }
};

//---

class App : public QWidget {
  Q_OBJECT

  Q_PROPERTY(bool debug          READ debug          WRITE setDebug         )
  Q_PROPERTY(int  animIterations READ animIterations WRITE setAnimIterations)

 public:
  App(QWidget *parent=0);
 ~App();

  bool debug() const { return debug_; }
  void setDebug(bool debug);

  int animIterations() const { return animIterations_; }
  void setAnimIterations(int i) { animIterations_ = i; }

  void addTimer();

  void reset();

  void addDrawCircle(const QRectF &r, const QColor &pen, const QColor &brush);

  void addDebugCircle(const QRectF &r, const QColor &pen, const QColor &brush);

 public slots:
  void factorEntered(int i);

 private:
  using Factors = std::vector<int>;

 private:
  void applyFactor();

  void saveOld();

  void addFadeOut();

  void resetFade();

  void animate();

  void calc();

  double s() const { return s_; }
  void setS(double s, double maxS) { s_ = s; maxS_ = maxS; }

  double maxS() const { return maxS_; }

  void paintEvent(QPaintEvent *);

  void resizeEvent(QResizeEvent *);

  void calcFactors(Circle *circle, const Factors &f);
  void calcPrime  (Circle *circle, int n);

  void generate();

  void animateStep();

  void draw(QPainter *painter);

 private slots:
  void animateSlot();

 private:
  friend class Circle;

  using DrawCircles = std::vector<DrawCircle>;

  int         factor_  { 1 };
  bool        debug_   { false };
  Factors     factors_;
  Circle*     circle_  { nullptr };
  double      s_       { 1.0 };
  double      maxS_    { 1.0 };
  QPointF     pos_;
  double      size_    { 1.0 };

  int     animIterations_ { 100 };
  QTimer *animateTimer_   { nullptr };
  int     animateCount_   { 0 };

  DrawCircles drawCircles_;
  DrawCircles debugCircles_;

  DrawCircles oldDrawCircles_;
  int         oldInd_  { 0 };
};

//---

struct CirclePoint {
  const Circle *circle { nullptr };
  QPointF       point;

  CirclePoint(const Circle *c, const QPointF &p) :
   circle(c), point(p) {
  }
};

//---

using CirclePoints = std::vector<CirclePoint>;
using Points       = std::vector<QPointF>;

//---

class Circle {
 public:
  static std::size_t lastId() { return lastIdRef(); }

  static void resetId() { lastIdRef() = 0; }

  Circle(App *app);
  Circle(App *app, Circle *parent, std::size_t n);

 ~Circle();

  Circle *parent() const { return parent_; }

  std::size_t id() const { return id_; }
  void setId(std::size_t n);

  std::size_t n() const { return n_; }

  double x() const { return c_.x(); }
  double y() const { return c_.y(); }
  double r() const { return r_; }

  double a() const { return a_; }
  void setA(double a) { a_ = a; }

  double xc() const { return xc_; }
  double yc() const { return yc_; }

  void addCircle(Circle *circle);

  void addPoint();

  void place();

  //double calcR() const;

  void fit();

  void move  (double x, double y);
  void moveBy(double dx, double dy);

  double closestCircleCircleDistance() const;

  double closestPointDistance() const;

  double closestSize() const;

  std::size_t size() const;

  QPointF center() const;

  void getPoints(Points &points) const;

  void getCirclePoints(CirclePoints &points) const;

  std::size_t numPoints() const { return points_.size(); }

  QPointF getPoint(int i) const;
  void setPoint(int i, const QPointF &p) { points_[size_t(i)] = p; }

  void generate(const QPointF &pos, double size);

 private:
  static std::size_t &lastIdRef() {
    static std::size_t lastId;

    return lastId;
  }

 private:
  App*        app_    { nullptr };   // parent app
  Circle*     parent_ { nullptr };   // parent circle (0 if none)
  std::size_t id_     { 0 };         // index (for color)
  std::size_t n_      { 0 };         // index in parent
  QPointF     c_;                    // center (0->1 (screen size))
  double      r_      { 0.5 };       // radius
  double      a_      { -M_PI/2.0 }; // angle
  Points      points_;               // offset from center (0-1)
  Circles     circles_;              // sub circles
  double      xc_     { 0.0 };
  double      yc_     { 0.0 };
};

}

#endif
