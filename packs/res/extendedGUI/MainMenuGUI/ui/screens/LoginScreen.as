package ui.screens
{
   import logging.*;
   import ui.*;
   import ui.components.*;
   
   public class LoginScreen extends Screen
   {
      private var loginWindow:LoginWindow;
      
      public function LoginScreen(id:String, depth:uint = 0)
      {
         super(id,depth);
      }
      
      override protected function unfreeze(... args) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"LoginScreen unfreeze",Navigator.NEWS_DIALOG);
         if(Navigator.NEWS_DIALOG != null)
         {
            Navigator.NEWS_DIALOG.doClose();
         }
      }
      
      override protected function init(... args) : void
      {
         appears = [];
         this.loginWindow = new LoginWindow(this);
         this.loginWindow.draggable = true;
         this.resize();
         this.loginWindow.setSize(460,350);
         this.loginWindow.x = (Base.stage.stageWidth - this.loginWindow.width) / 2;
         this.loginWindow.y = (Base.stage.stageHeight - this.loginWindow.height) / 2;
      }
      
      override protected function resize(... args) : void
      {
         this.loginWindow.x = (Base.stage.stageWidth - this.loginWindow.width) / 2;
         this.loginWindow.y = (Base.stage.stageHeight - this.loginWindow.height) / 2;
      }
      
      override public function set enabled(value:Boolean) : void
      {
         _enabled = value;
         this.loginWindow.enabled = _enabled;
      }
      
      override public function showUp() : void
      {
      }
      
      override public function showDown() : void
      {
      }
      
      override public function hideDown(onCompleteFunction:Function = null) : void
      {
         onCompleteFunction();
      }
      
      override public function hideUp(onCompleteFunction:Function = null) : void
      {
         onCompleteFunction();
      }
   }
}

