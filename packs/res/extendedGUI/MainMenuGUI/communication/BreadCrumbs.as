package communication
{
   import events.*;
   import flash.events.*;
   
   public class BreadCrumbs extends EventDispatcher
   {
      private static var _path:Vector.<BreadCrumb>;
      
      private static var _self:BreadCrumbs;
      
      public static const DIV:String = "/ ";
      
      _path = new Vector.<BreadCrumb>();
      
      public function BreadCrumbs(arg1:IEventDispatcher = null)
      {
         super(arg1);
      }
      
      public static function get Path() : Vector.<BreadCrumb>
      {
         return _path;
      }
      
      public static function set Path(arg1:Vector.<BreadCrumb>) : void
      {
         _path = arg1;
         self.dispatchEvent(new Event(Event.CHANGE));
      }
      
      public static function Add(arg1:BreadCrumb) : void
      {
         if(_path.indexOf(arg1) < 0)
         {
            _path.push(arg1);
            BreadCrumbs.getItemLabels();
            self.dispatchEvent(new BreadCrumbEvent(BreadCrumbEvent.ADDED,arg1));
         }
      }
      
      public static function Remove(arg1:BreadCrumb) : void
      {
         if(_path.indexOf(arg1) >= 0)
         {
            _path.splice(_path.indexOf(arg1),1);
            self.dispatchEvent(new BreadCrumbEvent(BreadCrumbEvent.REMOVED,arg1));
         }
         BreadCrumbs.getItemLabels();
      }
      
      public static function getItemLabels() : String
      {
         var loc1:* = "";
         var loc2:* = 0;
         while(loc2 < _path.length)
         {
            loc1 += _path[loc2].screen.label + "; ";
            loc2++;
         }
         return loc1;
      }
      
      public static function get self() : BreadCrumbs
      {
         if(!_self)
         {
            _self = new BreadCrumbs();
         }
         return _self;
      }
   }
}

