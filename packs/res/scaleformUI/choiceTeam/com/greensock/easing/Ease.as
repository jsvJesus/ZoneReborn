class com.greensock.easing.Ease
{
   var _func;
   var _params;
   var _type;
   var _power;
   static var _baseParams = [0,0,1,1];
   function Ease(func, extraParams, type, power)
   {
      this._func = func;
      this._params = !extraParams ? com.greensock.easing.Ease._baseParams : com.greensock.easing.Ease._baseParams.concat(extraParams);
      this._type = type || 0;
      this._power = power || 0;
   }
   function getRatio(p)
   {
      if(this._func)
      {
         this._params[0] = p;
         return this._func.apply(null,this._params);
      }
      var r = this._type !== 1 ? (this._type !== 2 ? (p >= 0.5 ? (1 - p) * 2 : p * 2) : p) : 1 - p;
      if(this._power === 1)
      {
         r *= r;
      }
      else if(this._power === 2)
      {
         r *= r * r;
      }
      else if(this._power === 3)
      {
         r *= r * r * r;
      }
      else if(this._power === 4)
      {
         r *= r * r * r * r;
      }
      return this._type !== 1 ? (this._type !== 2 ? (p >= 0.5 ? 1 - r / 2 : r / 2) : r) : 1 - r;
   }
}
