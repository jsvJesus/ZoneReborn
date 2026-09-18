package ui
{
   import flash.display.*;
   import flash.events.*;
   import flash.net.*;
   
   public class PremiumIcons
   {
      protected static var N:Number;
      
      protected static var Current:Number;
      
      protected static var Arr:Array;
      
      public static var Icons:Object = new Object();
      
      protected static var myLoader1:Loader = new Loader();
      
      public function PremiumIcons()
      {
         super();
      }
      
      public static function byId(id:int) : Bitmap
      {
         if(Icons[id.toString()] != null)
         {
            return new Bitmap(Icons[id.toString()],"auto",true);
         }
         return new Bitmap(new defaultIcon(),"auto",true);
      }
      
      public static function LoadIcons(Paths:Array) : *
      {
         var id:Number = NaN;
         N = Paths.length;
         Current = 0;
         Arr = Paths;
         NextIcon();
      }
      
      public static function LoadSettingsBackGround() : *
      {
         var url1:URLRequest = new URLRequest("../soGUI/maps/Login/setting_back.jpg");
         myLoader1.contentLoaderInfo.addEventListener(Event.COMPLETE,doneLoadSetting);
         myLoader1.load(url1);
      }
      
      protected static function NextIcon() : *
      {
         var iconLoader:Loader = null;
         iconLoader = new Loader();
         var url:URLRequest = new URLRequest(Arr[Current].Path);
         var handler:Function = function(e:Event):void
         {
            doneLoad(iconLoader,Arr[Current].ID);
         };
         iconLoader.contentLoaderInfo.addEventListener(Event.COMPLETE,handler);
         iconLoader.load(url);
      }
      
      public static function doneLoadSetting(e:Event) : *
      {
         Icons["Setting"] = Bitmap(myLoader1.content).bitmapData;
      }
      
      public static function doneLoad(ldr:Loader, i:Number) : *
      {
         Icons[i.toString()] = Bitmap(ldr.content).bitmapData;
         ++Current;
         if(Current < N)
         {
            NextIcon();
         }
      }
   }
}

