class mx.utils.Delegate extends Object
{
   var func;
   function Delegate(f)
   {
      super();
      this.func = f;
   }
   static function create(obj, func)
   {
      var f = function()
      {
         var target = arguments.callee.target;
         var func = arguments.callee.func;
         return func.apply(target,arguments);
      };
      f.target = obj;
      f.func = func;
      return f;
   }
   function createDelegate(obj)
   {
      return mx.utils.Delegate.create(obj,this.func);
   }
}
