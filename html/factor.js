'use strict';

window.addEventListener("load", eventWindowLoaded, false);

//------

var factors = new Factors();

function eventWindowLoaded () {
  if (canvasSupport()) {
    factors.init();
  }
}

function canvasSupport () {
  return true;
  //return Modernizr.canvas;
}

function assert(condition, message) {
  if (! condition) {
    throw message || "Assertion failed";
  }
}

function HSVtoRGB(h, s, v) {
  var r = v;
  var g = v;
  var b = v;

  if (s > 0.0) {
    while (h <    0.0) h += 360.0;
    while (h >= 360.0) h -= 360.0;

    h /= 60.0;

    var i = Math.floor(h);

    var f = h - i;

    var f1 = 1.0 - f;

    var p = v*(1.0 - s   );
    var q = v*(1.0 - s*f );
    var t = v*(1.0 - s*f1);

    switch (i) {
      case  0: r = v; g = t; b = p; break;
      case  1: r = q; g = v; b = p; break;
      case  2: r = p; g = v; b = t; break;
      case  3: r = p; g = q; b = v; break;
      case  4: r = t; g = p; b = v; break;
      case  5: r = v; g = p; b = q; break;
      default:                      break;
    }
  }

  return { "r":r, "g":g, "b":b };
}

function encodeRGB(r, g, b) {
  var s = "rgb(" + String(255*r) + "," + String(255*g) + "," + String(255*b) + ")";

  return s;
}

//------

function Factors () {
  this.prime   = new Prime();
  this.factor  = 1;
  this.debug   = false;
  this.factors = [];
  this.circle  = null;
  this.s       = 1.0;
  this.maxS    = 1.0;
  this.pos     = new Point(0, 0);
  this.size    = 1.0;
}

Factors.prototype.init = function() {
  this.lastId = 0;

  this.canvas = document.getElementById("canvas");
  this.gc     = this.canvas.getContext("2d");

  //---

  var valueNumber = document.getElementById("value");

  valueNumber.value = this.factor;

  valueNumber.addEventListener("change", this.valueChanged, false);

  var debugCheck = document.getElementById("debug");

  debugCheck.checked = false;

  debugCheck.addEventListener("change", this.debugChanged, false);

  //---

  this.calc();

  this.update();
};

Factors.prototype.valueChanged = function(e) {
  var value = e.target.value;

  var ivalue = parseInt(value, 10);

  if (! isNaN(ivalue))
    factors.factorEntered(ivalue);
};

Factors.prototype.debugChanged = function(e) {
  factors.debug = e.target.checked;

  factors.update();
};

Factors.prototype.reset = function() {
  this.circle = null;
};

Factors.prototype.factorEntered = function(i) {
console.log("Factors.prototype.factorEntered", i);
  this.factor = i;

  this.calc();

  this.update();
};

Factors.prototype.calc = function() {
  this.lastId = 0;

  this.reset();

  this.factors = this.prime.factors(this.factor);

  this.circle = new Circle(null, 0);

  if (this.prime.isPrime(this.factor))
    this.calcPrime(this.circle, this.factor);
  else
    this.calcFactors(this.circle, this.factors);

  this.circle.place();

  this.circle.fit();
console.log(this.factors);
};

Factors.prototype.calcFactors = function(circle, f) {
  if (f.length === 1) {
    this.calcPrime(circle, f[0]);
    return;
  }

  //------

  // split into first factor and list of remaining factors
  var n1 = f[0];

  var f1 = [];

  for (var i = 1; i < f.length; ++i)
    f1.push(f[i]);

  //------

  // add n circles
  for (var i = 0; i < n1; ++i) {
    var circle1 = new Circle(circle, i);

    circle.addCircle(circle1);

    this.calcFactors(circle1, f1);
  }
};

Factors.prototype.calcPrime = function(circle, n) {
  assert(n > 0);

  // reserve n ids
  circle.setId(n);

  // add n points
  for (var i = 0; i < n; ++i)
    circle.addPoint();
};

Factors.prototype.update = function() {
  this.draw();
};

Factors.prototype.draw = function() {
  this.gc.clearRect(0, 0, this.canvas.width, this.canvas.height);

  this.pos  = new Point(canvas.width/2.0, canvas.height/2.0);
  this.size = Math.min(canvas.width, canvas.height);

  this.circle.draw(this.pos, this.size);

  //---

  // draw number and factors
  var factorStr = String(this.factor);

  var nf = this.factors.length;

  var factorsStr = "";

  if (nf > 1) {
    for (var i = 0; i < nf; ++i) {
      if (i > 0) factorsStr += "x";

      factorsStr += String(this.factors[i]);
    }
  }
  else {
    factorsStr = "Prime";
  }

  //--

  var th = 24;
  var td = th*0.15;

  this.gc.font = String(th) + "px Arial";

  var tw1 = this.gc.measureText(factorStr ).width;
  var tw2 = this.gc.measureText(factorsStr).width;

  this.gc.fillStyle = "#ffffff80";

  this.gc.fillRect(20, 20, Math.max(tw1, tw2), 2*th);

  this.gc.fillStyle = "#000000";

  var td1 = 0.0;
  var td2 = 0.0;

  if (tw1 > tw2)
    td2 = (tw1 - tw2)/2.0;
  else
    td1 = (tw2 - tw1)/2.0;

  this.gc.fillText(factorStr , 20 + td1,   th + 20 - td);
  this.gc.fillText(factorsStr, 20 + td2, 2*th + 20 - td);
};

Factors.prototype.drawEllipse = function(x, y, w, h) {
  this.gc.beginPath();
  this.gc.ellipse(x + w/2, y + w/2, w/2, h/2, 0, 0, 2*Math.PI, false);
  this.gc.fill();
  this.gc.stroke();
};

//------

function CirclePoint (circle, point) {
  this.circle = circle;
  this.point  = point;
}

//------

function Circle (parent, n) {
  this.parent  = parent;
  this.id      = 0;
  this.n       = n;
  this.c       = new Point(0, 0);
  this.r       = 0.5;
  this.a       = Math.PI/2.0;
  this.points  = [];
  this.circles = [];
}

Circle.prototype.setId = function(n) {
  this.id = factors.lastId;

  factors.lastId += n;
};

Circle.prototype.addCircle = function(circle) {
  this.circles.push(circle);
};

Circle.prototype.addPoint = function() {
  this.points.push(new Point(0, 0));
};

Circle.prototype.place = function() {
console.log("Circle.prototype.place");
  if (this.circles.length > 0) {
    var nc = this.circles.length;

    var da = 2.0*Math.PI/nc;

    // place child circles
    var a = this.a;

    for (var i = 0; i < this.circles.length; ++i) {
      var circle = this.circles[i];

      if (this.size() === 2 && circle.size() === 2)
        circle.a = a + Math.PI/2.0;
      else
        circle.a = a;

      circle.place();

      a += da;
    }

    // find minimum point distance for child circles
    var d = 1E50;

    for (var i = 0; i < this.circles.length; ++i) {
      var circle = this.circles[i];

      d = Math.min(d, circle.closestPointDistance());
    }

    var rr = d/2.0;

    // place in circle (center (0.5, 0.5), radius 0.5)
    this.c = new Point(0.5, 0.5);
    this.r = 0.5;

    while (true) {
      a = this.a;

      for (var i = 0; i < this.circles.length; ++i) {
        var circle = this.circles[i];

        var x1 = this.c.x + this.r*Math.cos(a);
        var y1 = this.c.y + this.r*Math.sin(a);

        circle.move(x1, y1);

        a += da;
      }

      var r1 = this.closestCircleCircleDistance()/2;

      if (Math.abs(r1 - rr) < 1E-3)
        break;

      if (r1 < rr)
        this.r += 0.001;
      else
        this.r -= 0.001;
    }
  }
  else {
    var np = this.points.length;

    this.c = new Point(0.5, 0.5);
    this.r = 0.5;

    // place points in circle
    if (np > 1) {
      var a  = this.a;
      var da = 2.0*Math.PI/np;

      for (var i = 0; i < np; ++i) {
        var x1 = Math.cos(a);
        var y1 = Math.sin(a);

        this.points[i] = new Point(x1, y1);

        a += da;
      }
    }
    else {
      this.points[0] = new Point(0.0, 0.0);
    }
  }
};

Circle.prototype.fit = function() {
console.log("Circle.prototype.fit");
  // get all points
  var points = [];

  this.getPoints(points);

  // calc closest centers and range
  var np = points.length;

  var xmin = 0.5;
  var ymin = 0.5;
  var xmax = xmin;
  var ymax = ymin;

  var d = 2;

  for (var i = 0; i < np; ++i) {
    var p1 = points[i];

    for (var j = i + 1; j < np; ++j) {
      var p2 = points[j];

      var dx = p1.x - p2.x;
      var dy = p1.y - p2.y;

      var d1 = dx*dx + dy*dy;

      if (d1 < d)
        d = d1;
    }

    xmin = Math.min(xmin, p1.x);
    ymin = Math.min(ymin, p1.y);
    xmax = Math.max(xmax, p1.x);
    ymax = Math.max(ymax, p1.y);
  }

  // use closest center to defined size so points don't touch
  var s;

  if (d > 1E-6)
    s = Math.sqrt(d);
  else
    s = 1.0/np;

  xmin -= s/2.0;
  ymin -= s/2.0;
  xmax += s/2.0;
  ymax += s/2.0;

  var xs = xmax - xmin;
  var ys = ymax - ymin;

  var maxS = Math.max(xs, ys);

  factors.s    = s;
  factors.maxS = maxS;

  var xc = ((xmax + xmin)/2.0 - 0.5)/maxS + 0.5;
  var yc = ((ymax + ymin)/2.0 - 0.5)/maxS + 0.5;

  //this.c += new Point(xc - 0.5, yc - 0.5);

  this.moveBy(0.5 - xc, 0.5 - yc);
};

Circle.prototype.closestCircleCircleDistance = function() {
console.log("Circle.prototype.closestCircleCircleDistance");
  // get all points
  var points = [];

  this.getCirclePoints(points);

  // calc closest centers and range
  var np = points.length;

  var d = 1E50;

  for (var i = 0; i < np; ++i) {
    var p1 = points[i];

    for (var j = i + 1; j < np; ++j) {
      var p2 = points[j];

      if (p1.circle === p2.circle) continue;

      var dx = p1.point.x - p2.point.x;
      var dy = p1.point.y - p2.point.y;

      var d1 = dx*dx + dy*dy;

      if (d1 < d)
        d = d1;
    }
  }

  return Math.sqrt(d);
};

Circle.prototype.closestPointDistance = function() {
  // get all points
  var points = [];

  this.getPoints(points);

  // calc closest centers and range
  var np = points.length;

  var d = 1E50;

  for (var i = 0; i < np; ++i) {
    var p1 = points[i];

    for (var j = i + 1; j < np; ++j) {
      var p2 = points[j];

      var dx = p1.x - p2.x;
      var dy = p1.y - p2.y;

      var d1 = dx*dx + dy*dy;

      if (d1 < d)
        d = d1;
    }
  }

  return Math.sqrt(d);
};

Circle.prototype.closestSize = function() {
console.log("Circle.prototype.closestSize");
  var d = 1E50;

  if (this.circles.length > 0) {
    var nc = this.circles.length;

    for (var i = 0; i < nc; ++i) {
      var c1 = this.circles[i];

      var p1 = c1.center;

      for (var j = i + 1; j < nc; ++j) {
        var c2 = this.circles[j];

        var p2 = c2.center;

        var dx = p1.x - p2.x;
        var dy = p1.y - p2.y;

        var d1 = dx*dx + dy*dy;

        if (d1 < d)
          d = d1;
      }
    }
  }
  else {
    var np = this.points.length;

    for (var i = 0; i < np; ++i) {
      var p1 = this.points[i];

      for (var j = i + 1; j < np; ++j) {
        var p2 = this.points[j];

        var dx = p1.x - p2.x;
        var dy = p1.y - p2.y;

        var d1 = dx*dx + dy*dy;

        if (d1 < d)
          d = d1;
      }
    }
  }

  d = Math.sqrt(d);

  return d;
};

Circle.prototype.size = function() {
  return Math.max(this.circles.length, this.points.length);
};

Circle.prototype.getPoints = function(points) {
  for (var i = 0; i < this.circles.length; ++i)
    this.circles[i].getPoints(points);

  for (var i = 0; i < this.points.length; ++i)
    points.push(this.getPoint(i));
};

Circle.prototype.getCirclePoints = function(points) {
  for (var i = 0; i < this.circles.length; ++i)
    this.circles[i].getCirclePoints(points);

  for (var i = 0; i < this.points.length; ++i)
    points.push(new CirclePoint(this, this.getPoint(i)));
};

Circle.prototype.getPoint = function(i) {
  return new Point(this.c.x + this.r*this.points[i].x, this.c.y + this.r*this.points[i].y);
};

Circle.prototype.move = function(x, y) {
  var dx = x - this.c.x;
  var dy = y - this.c.y;

  this.moveBy(dx, dy);
};

Circle.prototype.moveBy = function(dx, dy) {
  this.c.x += dx;
  this.c.y += dy;

  for (var i = 0; i < this.circles.length; ++i)
    this.circles[i].moveBy(dx, dy);
};

Circle.prototype.draw = function(pos, size) {
  var ps = 8;

  var size1 = size/factors.maxS;

  if (this.circles.length > 0) {
    for (var i = 0; i < this.circles.length; ++i)
      this.circles[i].draw(pos, size);
  }
  else {
    var s = 0.9*factors.s*size1;

    // draw center
    if (factors.debug) {
      var xc = (this.c.x - 0.5)*size1 + pos.x;
      var yc = (this.c.y - 0.5)*size1 + pos.y;

      factors.gc.strokeStyle = "#00000000";
      factors.gc.fillStyle   = "#000000";

      factors.drawEllipse(xc - ps/2, yc - ps/2, ps, ps);
    }

    // draw point circles
    for (var i = 0; i < this.points.length; ++i) {
      var p = this.getPoint(i);

      var x = (p.x - 0.5)*size1 + pos.x;
      var y = (p.y - 0.5)*size1 + pos.y;

      // draw point circle
      var rgb = HSVtoRGB(360.0*(this.id + i)/factors.lastId, 0.6, 0.6);
      //c.setHsv((360.0*(this.id + i))/factors.lastId, 192, 192);

      factors.gc.strokeStyle = "#00000000";
      factors.gc.fillStyle   = encodeRGB(rgb.r, rgb.g, rgb.b);

      factors.drawEllipse(x - s/2, y - s/2, s, s);

      // draw point
      if (factors.debug) {
        factors.gc.strokeStyle = "#00000000";
        factors.gc.fillStyle   = "#000000";

        factors.drawEllipse(x - ps/2, y - ps/2, ps, ps);
      }
    }
  }

  // draw bounding circle
  if (factors.debug) {
    factors.gc.strokeStyle = "#000000";
    factors.gc.fillStyle   = "#00000000";

    var s = this.r*size1;

    var x = (this.c.x - 0.5)*size1 + pos.x;
    var y = (this.c.y - 0.5)*size1 + pos.y;

    factors.drawEllipse(x - s, y - s, 2*s, 2*s);
  }
};

//------

function Prime () {
  this.primes = new Set();

  this.primes.add(2);
  this.primes.add(3);
  this.primes.add(5);
  this.primes.add(7);

  this.primeMax = 10;
}

Prime.prototype.isPrime = function(i) {
  assert(i > 0);

  if (i <= 3)
    return true;

  this.genPrime(i);
 
  return this.primes.has(i);
};

Prime.prototype.genPrime = function(n) {
  if (n <= this.primeMax)
    return;

  for (var i = this.primeMax + 1; i <= n; ++i) {
    if (this.checkPrime(i))
      this.primes.add(i);
  }

  this.primeMax = n;
};

Prime.prototype.checkPrime = function(n) {
  for (var i of this.primes.values()) {
    if (n % i === 0)
      return false;
  }

  return true;
};

Prime.prototype.factors = function(n) {
  var v = [];

  if (this.isPrime(n))
    v.push(n);
  else {
    var n1 = Math.floor(n/2);

    for (var i = n1; i >= 2; --i) {
      if (! this.isPrime(i))
        continue;

      if (n % i != 0)
        continue;

      v.push(i);

      var v1 = this.factors(n / i);

      for (var i1 of v1)
        v.push(i1);

      break;
    }
  }

  return v;
};

//------

function Point (x, y) {
  this.x = x;
  this.y = y;
}
