class com.greensock.plugins.BezierPlugin extends com.greensock.plugins.TweenPlugin
{
   var _overwriteProps;
   var _func;
   var _round;
   var _target;
   var _props;
   var _timeRes;
   var _autoRotate;
   var _beziers;
   var _segCount;
   var _length;
   var _lengths;
   var _segments;
   var _l1;
   var _li;
   var _s1;
   var _si;
   var _l2;
   var _curSeg;
   var _s2;
   var _prec;
   var _initialRotations;
   var _startRatio;
   static var API = 2;
   static var _RAD2DEG = 57.29577951308232;
   static var _r1 = [];
   static var _r2 = [];
   static var _r3 = [];
   static var _corProps = {};
   function BezierPlugin()
   {
      super("bezier");
      this._overwriteProps.pop();
      this._func = {};
      this._round = {};
   }
   function _onInitTween(target, value, tween)
   {
      this._target = target;
      var vars = !(value instanceof Array) ? value : {values:value};
      this._props = [];
      this._timeRes = vars.timeResolution != null ? vars.timeResolution >> 0 : 6;
      var values = vars.values || [];
      var first = {};
      var second = values[0];
      var autoRotate = vars.autoRotate || tween.vars.orientToBezier;
      var p;
      var isFunc;
      var i;
      var j;
      var ar;
      var prepend;
      this._autoRotate = !autoRotate ? null : (!(autoRotate instanceof Array) ? [["_x","_y","_rotation",autoRotate !== true ? Number(autoRotate) : 0]] : [autoRotate][0]);
      for(p in second)
      {
         this._props.push(p);
      }
      i = this._props.length;
      while(--i > -1)
      {
         p = this._props[i];
         this._overwriteProps.push(p);
         isFunc = this._func[p] = typeof target[p] === "function";
         first[p] = !!isFunc ? target[!(p.indexOf("set") || typeof target["get" + p.substr(3)] !== "function") ? "get" + p.substr(3) : p]() : target[p];
         if(!prepend)
         {
            if(first[p] !== values[0][p])
            {
               prepend = first;
            }
         }
      }
      this._beziers = !(vars.type !== "cubic" && vars.type !== "quadratic" && vars.type !== "soft") ? com.greensock.plugins.BezierPlugin._parseBezierData(values,vars.type,first) : com.greensock.plugins.BezierPlugin.bezierThrough(values,!isNaN(vars.curviness) ? vars.curviness : 1,false,vars.type === "thruBasic",vars.correlate,prepend);
      this._segCount = this._beziers[p].length;
      if(this._timeRes)
      {
         var ld = com.greensock.plugins.BezierPlugin._parseLengthData(this._beziers,this._timeRes);
         this._length = ld.length;
         this._lengths = ld.lengths;
         this._segments = ld.segments;
         this._l1 = this._li = this._s1 = this._si = 0;
         this._l2 = this._lengths[0];
         this._curSeg = this._segments[0];
         this._s2 = this._curSeg[0];
         this._prec = 1 / this._curSeg.length;
      }
      if(ar = this._autoRotate)
      {
         this._initialRotations = [];
         if(!(ar[0] instanceof Array))
         {
            this._autoRotate = ar = [ar];
         }
         i = ar.length;
         while(--i > -1)
         {
            j = 0;
            while(j < 3)
            {
               p = ar[i][j];
               this._func[p] = typeof target[p] !== "function" ? false : target[!(p.indexOf("set") || typeof target["get" + p.substr(3)] !== "function") ? "get" + p.substr(3) : p];
               j++;
            }
            p = ar[i][2];
            this._initialRotations[i] = !this._func[p] ? this._target[p] : this._func[p]();
         }
      }
      this._startRatio = !tween.vars.runBackwards ? 0 : 1;
      return true;
   }
   static function bezierThrough(values, curviness, quadratic, basic, correlate, prepend)
   {
      if(curviness == null)
      {
         curviness = 1;
      }
      var obj = {};
      var props = [];
      var first = prepend || values[0];
      var i;
      var p;
      var j;
      var a;
      var l;
      var r;
      var seamless;
      var last;
      correlate = typeof correlate !== "string" ? ",_x,_y,x,y,z," : "," + correlate + ",";
      for(p in values[0])
      {
         props.push(p);
      }
      if(values.length > 1)
      {
         last = values[values.length - 1];
         seamless = true;
         i = props.length;
         while(--i > -1)
         {
            p = props[i];
            if(Math.abs(first[p] - last[p]) > 0.05)
            {
               seamless = false;
               break;
            }
         }
         if(seamless)
         {
            values = values.concat();
            if(prepend)
            {
               values.unshift(prepend);
            }
            values.push(values[1]);
            prepend = values[values.length - 3];
         }
      }
      com.greensock.plugins.BezierPlugin._r1.length = com.greensock.plugins.BezierPlugin._r2.length = com.greensock.plugins.BezierPlugin._r3.length = 0;
      i = props.length;
      while(--i > -1)
      {
         p = props[i];
         com.greensock.plugins.BezierPlugin._corProps[p] = correlate.indexOf("," + p + ",") !== -1;
         obj[p] = com.greensock.plugins.BezierPlugin._parseAnchors(values,p,com.greensock.plugins.BezierPlugin._corProps[p],prepend);
      }
      i = com.greensock.plugins.BezierPlugin._r1.length;
      while(--i > -1)
      {
         com.greensock.plugins.BezierPlugin._r1[i] = Math.sqrt(com.greensock.plugins.BezierPlugin._r1[i]);
         com.greensock.plugins.BezierPlugin._r2[i] = Math.sqrt(com.greensock.plugins.BezierPlugin._r2[i]);
      }
      if(!basic)
      {
         i = props.length;
         while(--i > -1)
         {
            if(com.greensock.plugins.BezierPlugin._corProps[p])
            {
               a = obj[props[i]];
               l = a.length - 1;
               j = 0;
               while(j < l)
               {
                  r = a[j + 1].da / com.greensock.plugins.BezierPlugin._r2[j] + a[j].da / com.greensock.plugins.BezierPlugin._r1[j];
                  com.greensock.plugins.BezierPlugin._r3[j] = (com.greensock.plugins.BezierPlugin._r3[j] || 0) + r * r;
                  j++;
               }
            }
         }
         i = com.greensock.plugins.BezierPlugin._r3.length;
         while(--i > -1)
         {
            com.greensock.plugins.BezierPlugin._r3[i] = Math.sqrt(com.greensock.plugins.BezierPlugin._r3[i]);
         }
      }
      i = props.length;
      j = !quadratic ? 1 : 4;
      while(--i > -1)
      {
         p = props[i];
         a = obj[p];
         com.greensock.plugins.BezierPlugin._calculateControlPoints(a,curviness,quadratic,basic,com.greensock.plugins.BezierPlugin._corProps[p]);
         if(seamless)
         {
            a.splice(0,j);
            a.splice(a.length - j,j);
         }
      }
      return obj;
   }
   static function _parseBezierData(values, type, prepend)
   {
      type = type || "soft";
      var obj = {};
      var inc = type !== "cubic" ? 2 : 3;
      var soft = type === "soft";
      var a;
      var b;
      var c;
      var d;
      var cur;
      var props;
      var i;
      var j;
      var l;
      var p;
      var cnt;
      var tmp;
      if(soft && prepend)
      {
         values = [prepend].concat(values);
      }
      if(values == null || values.length < inc + 1)
      {
      }
      props = [];
      for(p in values[0])
      {
         props.push(p);
      }
      i = props.length;
      while(--i > -1)
      {
         p = props[i];
         obj[p] = cur = [];
         cnt = 0;
         l = values.length;
         j = 0;
         while(j < l)
         {
            a = prepend != null ? (!(typeof (tmp = values[j][p]) === "string" && tmp.charAt(1) === "=") ? Number(tmp) : prepend[p] + Number(tmp.charAt(0) + tmp.substr(2))) : values[j][p];
            if(soft)
            {
               if(j > 1)
               {
                  if(j < l - 1)
                  {
                     cur[cnt++] = (a + cur[cnt - 2]) / 2;
                  }
               }
            }
            cur[cnt++] = a;
            j++;
         }
         l = cnt - inc + 1;
         cnt = 0;
         j = 0;
         while(j < l)
         {
            a = cur[j];
            b = cur[j + 1];
            c = cur[j + 2];
            d = inc !== 2 ? cur[j + 3] : 0;
            cur[cnt++] = inc !== 3 ? new com.greensock.plugins.core.Segment(a,(2 * b + a) / 3,(2 * b + c) / 3,c) : new com.greensock.plugins.core.Segment(a,b,c,d);
            j += inc;
         }
         cur.length = cnt;
      }
      return obj;
   }
   static function _parseAnchors(values, p, correlate, prepend)
   {
      var a = [];
      var l;
      var i;
      var p1;
      var p2;
      var p3;
      var tmp;
      if(prepend)
      {
         values = [prepend].concat(values);
         i = values.length;
         while(--i > -1)
         {
            if(typeof (tmp = values[i][p]) === "string")
            {
               if(tmp.charAt(1) === "=")
               {
                  values[i][p] = prepend[p] + Number(tmp.charAt(0) + tmp.substr(2));
               }
            }
         }
      }
      l = values.length - 2;
      if(l < 0)
      {
         a[0] = new com.greensock.plugins.core.Segment(values[0][p],0,0,values[l >= -1 ? 1 : 0][p]);
         return a;
      }
      i = 0;
      while(i < l)
      {
         p1 = values[i][p];
         p2 = values[i + 1][p];
         a[i] = new com.greensock.plugins.core.Segment(p1,0,0,p2);
         if(correlate)
         {
            p3 = values[i + 2][p];
            com.greensock.plugins.BezierPlugin._r1[i] = (com.greensock.plugins.BezierPlugin._r1[i] || 0) + (p2 - p1) * (p2 - p1);
            com.greensock.plugins.BezierPlugin._r2[i] = (com.greensock.plugins.BezierPlugin._r2[i] || 0) + (p3 - p2) * (p3 - p2);
         }
         i++;
      }
      a[i] = new com.greensock.plugins.core.Segment(values[i][p],0,0,values[i + 1][p]);
      return a;
   }
   static function _calculateControlPoints(a, curviness, quad, basic, correlate)
   {
      var l = a.length - 1;
      var ii = 0;
      var cp1 = a[0].a;
      var i;
      var p1;
      var p2;
      var p3;
      var seg;
      var m1;
      var m2;
      var mm;
      var cp2;
      var qb;
      var r1;
      var r2;
      var tl;
      i = 0;
      while(i < l)
      {
         seg = a[ii];
         p1 = seg.a;
         p2 = seg.d;
         p3 = a[ii + 1].d;
         if(correlate)
         {
            r1 = com.greensock.plugins.BezierPlugin._r1[i];
            r2 = com.greensock.plugins.BezierPlugin._r2[i];
            tl = (r2 + r1) * curviness * 0.25 / (!basic ? com.greensock.plugins.BezierPlugin._r3[i] || 0.5 : 0.5);
            m1 = p2 - (p2 - p1) * (!basic ? (r1 === 0 ? 0 : tl / r1) : curviness * 0.5);
            m2 = p2 + (p3 - p2) * (!basic ? (r2 === 0 ? 0 : tl / r2) : curviness * 0.5);
            mm = p2 - (m1 + ((m2 - m1) * (r1 * 3 / (r1 + r2) + 0.5) / 4 || 0));
         }
         else
         {
            m1 = p2 - (p2 - p1) * curviness * 0.5;
            m2 = p2 + (p3 - p2) * curviness * 0.5;
            mm = p2 - (m1 + m2) / 2;
         }
         m1 += mm;
         m2 += mm;
         seg.c = cp2 = m1;
         if(i != 0)
         {
            seg.b = cp1;
         }
         else
         {
            seg.b = cp1 = seg.a + (seg.c - seg.a) * 0.6;
         }
         seg.da = p2 - p1;
         seg.ca = cp2 - p1;
         seg.ba = cp1 - p1;
         if(quad)
         {
            qb = com.greensock.plugins.BezierPlugin.cubicToQuadratic(p1,cp1,cp2,p2);
            a.splice(ii,1,qb[0],qb[1],qb[2],qb[3]);
            ii += 4;
         }
         else
         {
            ii++;
         }
         cp1 = m2;
         i++;
      }
      seg = a[ii];
      seg.b = cp1;
      seg.c = cp1 + (seg.d - cp1) * 0.4;
      seg.da = seg.d - seg.a;
      seg.ca = seg.c - seg.a;
      seg.ba = cp1 - seg.a;
      if(quad)
      {
         qb = com.greensock.plugins.BezierPlugin.cubicToQuadratic(seg.a,cp1,seg.c,seg.d);
         a.splice(ii,1,qb[0],qb[1],qb[2],qb[3]);
      }
   }
   static function cubicToQuadratic(a, b, c, d)
   {
      var q1 = {a:a};
      var q2 = {};
      var q3 = {};
      var q4 = {c:d};
      var mab = (a + b) / 2;
      var mbc = (b + c) / 2;
      var mcd = (c + d) / 2;
      var mabc = (mab + mbc) / 2;
      var mbcd = (mbc + mcd) / 2;
      var m8 = (mbcd - mabc) / 8;
      q1.b = mab + (a - mab) / 4;
      q2.b = mabc + m8;
      q1.c = q2.a = (q1.b + q2.b) / 2;
      q2.c = q3.a = (mabc + mbcd) / 2;
      q3.b = mbcd - m8;
      q4.b = mcd + (d - mcd) / 4;
      q3.c = q4.a = (q3.b + q4.b) / 2;
      return [q1,q2,q3,q4];
   }
   static function quadraticToCubic(a, b, c)
   {
      return new com.greensock.plugins.core.Segment(a,(2 * b + a) / 3,(2 * b + c) / 3,c);
   }
   static function _parseLengthData(obj, precision)
   {
      if(precision == null)
      {
         precision = 6;
      }
      var a = [];
      var lengths = [];
      var d = 0;
      var total = 0;
      var threshold = precision - 1;
      var segments = [];
      var curLS = [];
      var p;
      var i;
      var l;
      var index;
      for(p in obj)
      {
         com.greensock.plugins.BezierPlugin._addCubicLengths(obj[p],a,precision);
      }
      l = a.length;
      i = 0;
      while(i < l)
      {
         d += Math.sqrt(a[i]);
         index = i % precision;
         curLS[index] = d;
         if(index == threshold)
         {
            total += d;
            index = i / precision >> 0;
            segments[index] = curLS;
            lengths[index] = total;
            d = 0;
            curLS = [];
         }
         i++;
      }
      return {length:total,lengths:lengths,segments:segments};
   }
   static function _addCubicLengths(a, steps, precision)
   {
      if(precision == null)
      {
         precision = 6;
      }
      var inc = 1 / precision;
      var j = a.length;
      var d;
      var d1;
      var s;
      var da;
      var ca;
      var ba;
      var p;
      var i;
      var inv;
      var bez;
      var index;
      while(--j > -1)
      {
         bez = a[j];
         s = bez.a;
         da = bez.d - s;
         ca = bez.c - s;
         ba = bez.b - s;
         d = d1 = 0;
         i = 1;
         while(i <= precision)
         {
            p = inc * i;
            inv = 1 - p;
            d = d1 - (d1 = (p * p * da + 3 * inv * (p * ca + inv * ba)) * p);
            index = j * precision + i - 1;
            steps[index] = (steps[index] || 0) + d * d;
            i++;
         }
      }
   }
   function _kill(lookup)
   {
      var a = this._props;
      var p;
      var i;
      for(p in this._beziers)
      {
         if(lookup[p] != null)
         {
            delete this._beziers[p];
            delete this._func[p];
            i = a.length;
            while(--i > -1)
            {
               if(a[i] === p)
               {
                  a.splice(i,1);
               }
            }
         }
      }
      return super._kill(lookup);
   }
   function _roundProps(lookup, value)
   {
      var op = this._overwriteProps;
      var i = op.length;
      while(--i > -1)
      {
         if(lookup[op[i]] != null || lookup.bezier || lookup.bezierThrough)
         {
            this._round[op[i]] = value;
         }
      }
   }
   function setRatio(v)
   {
      var segments = this._segCount;
      var func = this._func;
      var target = this._target;
      var notStart = v !== this._startRatio;
      var curIndex;
      var inv;
      var i;
      var p;
      var b;
      var t;
      var val;
      var l;
      var lengths;
      var curSeg;
      if(this._timeRes == 0)
      {
         curIndex = v >= 0 ? (v < 1 ? segments * v >> 0 : segments - 1) : 0;
         t = (v - curIndex * (1 / segments)) * segments;
      }
      else
      {
         lengths = this._lengths;
         curSeg = this._curSeg;
         v *= this._length;
         i = this._li;
         if(v > this._l2 && i < segments - 1)
         {
            l = segments - 1;
            while(i < l && (this._l2 = lengths[++i]) <= v)
            {
            }
            this._l1 = lengths[i - 1];
            this._li = i;
            this._curSeg = curSeg = this._segments[i];
            this._s2 = curSeg[this._s1 = this._si = 0];
         }
         else if(v < this._l1 && i > 0)
         {
            while(i > 0 && (this._l1 = lengths[--i]) >= v)
            {
            }
            if(i === 0 && v < this._l1)
            {
               this._l1 = 0;
            }
            else
            {
               i++;
            }
            this._l2 = lengths[i];
            this._li = i;
            this._curSeg = curSeg = this._segments[i];
            this._s1 = curSeg[(this._si = curSeg.length - 1) - 1] || 0;
            this._s2 = curSeg[this._si];
         }
         curIndex = i;
         v -= this._l1;
         i = this._si;
         if(v > this._s2 && i < curSeg.length - 1)
         {
            l = curSeg.length - 1;
            while(i < l && (this._s2 = curSeg[++i]) <= v)
            {
            }
            this._s1 = curSeg[i - 1];
            this._si = i;
         }
         else if(v < this._s1 && i > 0)
         {
            while(i > 0 && (this._s1 = curSeg[--i]) >= v)
            {
            }
            if(i === 0 && v < this._s1)
            {
               this._s1 = 0;
            }
            else
            {
               i++;
            }
            this._s2 = curSeg[i];
            this._si = i;
         }
         t = (i + (v - this._s1) / (this._s2 - this._s1)) * this._prec;
      }
      inv = 1 - t;
      i = this._props.length;
      while(--i > -1)
      {
         p = this._props[i];
         b = this._beziers[p][curIndex];
         val = (t * t * b.da + 3 * inv * (t * b.ca + inv * b.ba)) * t + b.a;
         if(this._round[p])
         {
            val = val + (val <= 0 ? -0.5 : 0.5) >> 0;
         }
         if(func[p])
         {
            target[p](val);
         }
         else
         {
            target[p] = val;
         }
      }
      if(this._autoRotate != null)
      {
         var ar = this._autoRotate;
         var b2;
         var x1;
         var y1;
         var x2;
         var y2;
         var add;
         var conv;
         i = ar.length;
         while(--i > -1)
         {
            p = ar[i][2];
            add = ar[i][3] || 0;
            conv = ar[i][4] != true ? com.greensock.plugins.BezierPlugin._RAD2DEG : 1;
            b = this._beziers[ar[i][0]][curIndex];
            b2 = this._beziers[ar[i][1]][curIndex];
            x1 = b.a + (b.b - b.a) * t;
            x2 = b.b + (b.c - b.b) * t;
            x1 += (x2 - x1) * t;
            x2 += (b.c + (b.d - b.c) * t - x2) * t;
            y1 = b2.a + (b2.b - b2.a) * t;
            y2 = b2.b + (b2.c - b2.b) * t;
            y1 += (y2 - y1) * t;
            y2 += (b2.c + (b2.d - b2.c) * t - y2) * t;
            val = !notStart ? this._initialRotations[i] : Math.atan2(y2 - y1,x2 - x1) * conv + add;
            if(func[p])
            {
               target[p](val);
            }
            else
            {
               target[p] = val;
            }
         }
      }
   }
}
