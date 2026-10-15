class com.greensock.plugins.ColorMatrixFilterPlugin extends com.greensock.plugins.FilterPlugin
{
   var _matrix;
   var _filter;
   var _matrixTween;
   static var API = 2;
   static var _propNames = [];
   static var _idMatrix = [1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0];
   static var _lumR = 0.212671;
   static var _lumG = 0.71516;
   static var _lumB = 0.072169;
   function ColorMatrixFilterPlugin()
   {
      super("colorMatrixFilter");
   }
   function _onInitTween(target, value, tween)
   {
      var cmf = value;
      this._initFilter(target,{remove:value.remove,index:value.index,addFilter:value.addFilter},tween,flash.filters.ColorMatrixFilter,new flash.filters.ColorMatrixFilter(com.greensock.plugins.ColorMatrixFilterPlugin._idMatrix.slice()),com.greensock.plugins.ColorMatrixFilterPlugin._propNames);
      this._matrix = flash.filters.ColorMatrixFilter(this._filter).matrix;
      var endMatrix = [];
      if(cmf.matrix != null && cmf.matrix instanceof Array)
      {
         endMatrix = cmf.matrix;
      }
      else
      {
         if(cmf.relative)
         {
            endMatrix = this._matrix.slice();
         }
         else
         {
            endMatrix = com.greensock.plugins.ColorMatrixFilterPlugin._idMatrix.slice();
         }
         endMatrix = com.greensock.plugins.ColorMatrixFilterPlugin.setBrightness(endMatrix,cmf.brightness);
         endMatrix = com.greensock.plugins.ColorMatrixFilterPlugin.setContrast(endMatrix,cmf.contrast);
         endMatrix = com.greensock.plugins.ColorMatrixFilterPlugin.setHue(endMatrix,cmf.hue);
         endMatrix = com.greensock.plugins.ColorMatrixFilterPlugin.setSaturation(endMatrix,cmf.saturation);
         endMatrix = com.greensock.plugins.ColorMatrixFilterPlugin.setThreshold(endMatrix,cmf.threshold);
         if(!isNaN(cmf.colorize))
         {
            endMatrix = com.greensock.plugins.ColorMatrixFilterPlugin.colorize(endMatrix,cmf.colorize,cmf.amount);
         }
      }
      this._matrixTween = new com.greensock.plugins.EndArrayPlugin();
      this._matrixTween._init(this._matrix,endMatrix);
      return true;
   }
   function setRatio(v)
   {
      this._matrixTween.setRatio(v);
      flash.filters.ColorMatrixFilter(this._filter).matrix = this._matrix;
      super.setRatio(v);
   }
   static function colorize(m, color, amount)
   {
      if(isNaN(color))
      {
         return m;
      }
      if(isNaN(amount))
      {
         amount = 1;
      }
      var r = (color >> 16 & 0xFF) / 255;
      var g = (color >> 8 & 0xFF) / 255;
      var b = (color & 0xFF) / 255;
      var inv = 1 - amount;
      var temp = [inv + amount * r * com.greensock.plugins.ColorMatrixFilterPlugin._lumR,amount * r * com.greensock.plugins.ColorMatrixFilterPlugin._lumG,amount * r * com.greensock.plugins.ColorMatrixFilterPlugin._lumB,0,0,amount * g * com.greensock.plugins.ColorMatrixFilterPlugin._lumR,inv + amount * g * com.greensock.plugins.ColorMatrixFilterPlugin._lumG,amount * g * com.greensock.plugins.ColorMatrixFilterPlugin._lumB,0,0,amount * b * com.greensock.plugins.ColorMatrixFilterPlugin._lumR,amount * b * com
      .greensock.plugins.ColorMatrixFilterPlugin._lumG,inv + amount * b * com.greensock.plugins.ColorMatrixFilterPlugin._lumB,0,0,0,0,0,1,0];
      return com.greensock.plugins.ColorMatrixFilterPlugin.applyMatrix(temp,m);
   }
   static function setThreshold(m, n)
   {
      if(isNaN(n))
      {
         return m;
      }
      var temp = [com.greensock.plugins.ColorMatrixFilterPlugin._lumR * 256,com.greensock.plugins.ColorMatrixFilterPlugin._lumG * 256,com.greensock.plugins.ColorMatrixFilterPlugin._lumB * 256,0,-256 * n,com.greensock.plugins.ColorMatrixFilterPlugin._lumR * 256,com.greensock.plugins.ColorMatrixFilterPlugin._lumG * 256,com.greensock.plugins.ColorMatrixFilterPlugin._lumB * 256,0,-256 * n,com.greensock.plugins.ColorMatrixFilterPlugin._lumR * 256,com.greensock.plugins.ColorMatrixFilterPlugin._lumG * 256,com
      .greensock.plugins.ColorMatrixFilterPlugin._lumB * 256,0,-256 * n,0,0,0,1,0];
      return com.greensock.plugins.ColorMatrixFilterPlugin.applyMatrix(temp,m);
   }
   static function setHue(m, n)
   {
      if(isNaN(n))
      {
         return m;
      }
      n *= 0.017453292519943295;
      var c = Math.cos(n);
      var s = Math.sin(n);
      var temp = [com.greensock.plugins.ColorMatrixFilterPlugin._lumR + c * (1 - com.greensock.plugins.ColorMatrixFilterPlugin._lumR) + s * (- com.greensock.plugins.ColorMatrixFilterPlugin._lumR),com.greensock.plugins.ColorMatrixFilterPlugin._lumG + c * (- com.greensock.plugins.ColorMatrixFilterPlugin._lumG) + s * (- com.greensock.plugins.ColorMatrixFilterPlugin._lumG),com.greensock.plugins.ColorMatrixFilterPlugin._lumB + c * (- com.greensock.plugins.ColorMatrixFilterPlugin._lumB) + s * (1 - com.greensock
      .plugins.ColorMatrixFilterPlugin._lumB),0,0,com.greensock.plugins.ColorMatrixFilterPlugin._lumR + c * (- com.greensock.plugins.ColorMatrixFilterPlugin._lumR) + s * 0.143,com.greensock.plugins.ColorMatrixFilterPlugin._lumG + c * (1 - com.greensock.plugins.ColorMatrixFilterPlugin._lumG) + s * 0.14,com.greensock.plugins.ColorMatrixFilterPlugin._lumB + c * (- com.greensock.plugins.ColorMatrixFilterPlugin._lumB) + s * -0.283,0,0,com.greensock.plugins.ColorMatrixFilterPlugin._lumR + c * (- com.greensock
      .plugins.ColorMatrixFilterPlugin._lumR) + s * (- (1 - com.greensock.plugins.ColorMatrixFilterPlugin._lumR)),com.greensock.plugins.ColorMatrixFilterPlugin._lumG + c * (- com.greensock.plugins.ColorMatrixFilterPlugin._lumG) + s * com.greensock.plugins.ColorMatrixFilterPlugin._lumG,com.greensock.plugins.ColorMatrixFilterPlugin._lumB + c * (1 - com.greensock.plugins.ColorMatrixFilterPlugin._lumB) + s * com.greensock.plugins.ColorMatrixFilterPlugin._lumB,0,0,0,0,0,1,0,0,0,0,0,1];
      return com.greensock.plugins.ColorMatrixFilterPlugin.applyMatrix(temp,m);
   }
   static function setBrightness(m, n)
   {
      if(isNaN(n))
      {
         return m;
      }
      n = n * 100 - 100;
      return com.greensock.plugins.ColorMatrixFilterPlugin.applyMatrix([1,0,0,0,n,0,1,0,0,n,0,0,1,0,n,0,0,0,1,0,0,0,0,0,1],m);
   }
   static function setSaturation(m, n)
   {
      if(isNaN(n))
      {
         return m;
      }
      var inv = 1 - n;
      var r = inv * com.greensock.plugins.ColorMatrixFilterPlugin._lumR;
      var g = inv * com.greensock.plugins.ColorMatrixFilterPlugin._lumG;
      var b = inv * com.greensock.plugins.ColorMatrixFilterPlugin._lumB;
      var temp = [r + n,g,b,0,0,r,g + n,b,0,0,r,g,b + n,0,0,0,0,0,1,0];
      return com.greensock.plugins.ColorMatrixFilterPlugin.applyMatrix(temp,m);
   }
   static function setContrast(m, n)
   {
      if(isNaN(n))
      {
         return m;
      }
      n += 0.01;
      var temp = [n,0,0,0,128 * (1 - n),0,n,0,0,128 * (1 - n),0,0,n,0,128 * (1 - n),0,0,0,1,0];
      return com.greensock.plugins.ColorMatrixFilterPlugin.applyMatrix(temp,m);
   }
   static function applyMatrix(m, m2)
   {
      if(!(m instanceof Array) || !(m2 instanceof Array))
      {
         return m2;
      }
      var temp = [];
      var i = 0;
      var z = 0;
      var y;
      var x;
      y = 0;
      while(y < 4)
      {
         x = 0;
         while(x < 5)
         {
            z = x != 4 ? 0 : m[i + 4];
            temp[i + x] = m[i] * m2[x] + m[i + 1] * m2[x + 5] + m[i + 2] * m2[x + 10] + m[i + 3] * m2[x + 15] + z;
            x++;
         }
         i += 5;
         y++;
      }
      return temp;
   }
}
