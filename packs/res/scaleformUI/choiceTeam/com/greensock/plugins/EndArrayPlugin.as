class com.greensock.plugins.EndArrayPlugin extends com.greensock.plugins.TweenPlugin
{
   var _info;
   var _a;
   var _round;
   static var API = 2;
   function EndArrayPlugin()
   {
      super("endArray");
      this._info = [];
   }
   function _onInitTween(target, value, tween)
   {
      if(!(target instanceof Array) || !(value instanceof Array))
      {
         return false;
      }
      this._init(Array(target),Array(value));
      return true;
   }
   function _init(start, end)
   {
      this._a = start;
      var i = end.length;
      var cnt = 0;
      while(--i > -1)
      {
         if(start[i] != end[i] && start[i] != null)
         {
            this._info[cnt++] = {i:i,s:this._a[i],c:end[i] - this._a[i]};
         }
      }
   }
   function _roundProps(lookup, value)
   {
      if(lookup.endArray)
      {
         this._round = value;
      }
   }
   function setRatio(v)
   {
      var i = this._info.length;
      var ti;
      var val;
      if(this._round)
      {
         while(--i > -1)
         {
            ti = this._info[i];
            this._a[ti.i] = (val = ti.c * v + ti.s) <= 0 ? val - 0.5 >> 0 : val + 0.5 >> 0;
         }
      }
      else
      {
         while(--i > -1)
         {
            ti = this._info[i];
            this._a[ti.i] = ti.c * v + ti.s;
         }
      }
   }
}
