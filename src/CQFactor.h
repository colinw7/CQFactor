#ifndef CQFactor_H
#define CQFactor_H

#include <CCircleFactor.h>
#include <QWidget>

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

 Q_SIGNALS:
  void factorEntered(int i);

 private Q_SLOTS:
  void factorSlot();
  void debugSlot(int);

 private:
  App*      app_  { nullptr };
  QSpinBox* edit_ { nullptr };
};

//------

struct DrawCircleData {
  QRectF rect;
  QColor pen;
  QColor brush;

  DrawCircleData() = default;

  DrawCircleData(const QRectF &rect, const QColor &pen, const QColor &brush) :
   rect(rect), pen(pen), brush(brush) {
  }
};

struct DrawCircle {
  DrawCircleData data;
  DrawCircleData oldData;

  DrawCircle() = default;

  DrawCircle(const QRectF &rect, const QColor &pen, const QColor &brush) :
   data(rect, pen, brush) {
  }
};

//---

class AppCircleMgr;

class App : public QWidget {
  Q_OBJECT

  Q_PROPERTY(bool   debug          READ isDebug        WRITE setDebug         )
  Q_PROPERTY(int    animIterations READ animIterations WRITE setAnimIterations)
  Q_PROPERTY(double hsvSaturation  READ hsvSaturation  WRITE setHsvSaturation )
  Q_PROPERTY(double hsvValue       READ hsvValue       WRITE setHsvValue      )

 public:
  App(QWidget *parent=0);
 ~App();

  bool isDebug() const { return debug_; }
  void setDebug(bool debug);

  int animIterations() const { return animIterations_; }
  void setAnimIterations(int i) { animIterations_ = i; }

  double hsvSaturation() const { return hsvSaturation_; }
  void setHsvSaturation(double r) { hsvSaturation_ = r; }

  double hsvValue() const { return hsvValue_; }
  void setHsvValue(double r) { hsvValue_ = r; }

  AppCircleMgr *circleMgr() { return circleMgr_; }

  void addTimer();

  void reset();

  void addDrawCircle(const QRectF &r, const QColor &pen, const QColor &brush);

  void addDebugCircle(const QRectF &r, const QColor &pen, const QColor &brush);

 public Q_SLOTS:
  void factorEntered(int i);

 private:
  void applyFactor();

  void saveOld();

  void addFadeOut();

  void resetFade();

  void animate();

  void calc();

  void paintEvent(QPaintEvent *);

  void resizeEvent(QResizeEvent *);

  void generate();

  void animateStep();

  void draw(QPainter *painter);

 private Q_SLOTS:
  void animateSlot();

 private:
  friend class Circle;

  using DrawCircles = std::vector<DrawCircle>;

  bool debug_ { false };

  double hsvSaturation_ { 0.6 };
  double hsvValue_      { 0.6 };

  int     animIterations_ { 100 };
  QTimer *animateTimer_   { nullptr };
  int     animateCount_   { 0 };

  AppCircleMgr *circleMgr_ { nullptr };

  DrawCircles drawCircles_;
  DrawCircles debugCircles_;

  DrawCircles oldDrawCircles_;
  int         oldInd_  { 0 };
};

//---

class AppCircleMgr : public CCircleFactor::CircleMgr {
 public:
  AppCircleMgr(App *app) :
   app_(app) {
  }

  void addDrawCircle(double xc, double yc, double size, double f) override {
    QColor c;

    double s = app_->hsvSaturation();
    double v = app_->hsvValue();

    c.setHsv(int(f*360.0), int(s*255.0), int(v*255.0));

    app_->addDrawCircle(QRectF(xc - size/2, yc - size/2, size, size), QColor(0, 0, 0, 0), c);
  }

  void addDebugCircle(double xc, double yc, double size, double strokeAlpha,
                      double fillAlpha) override {
    app_->addDebugCircle(QRectF(xc - size/2, yc - size/2, size, size),
                         QColor(0, 0, 0, int(255*strokeAlpha)),
                         QColor(0, 0, 0, int(255*fillAlpha)));
  }

 private:
  App *app_ { nullptr };
};

}

#endif
