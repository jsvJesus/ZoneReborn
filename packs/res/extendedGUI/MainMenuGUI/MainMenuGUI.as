package
{
   import com.dvalimona.components.*;
   import communication.*;
   import events.*;
   import flash.events.*;
   import flash.ui.*;
   import logging.*;
   import ui.*;
   import ui.screens.*;
   
   public class MainMenuGUI extends Base
   {
      public static const TEST_SCREEN:String = "test_screen";
      
      public static const LOGIN_SCREEN:String = "login_screen";
      
      public static const CHARNAME_SCREEN:String = "char_name_screen";
      
      public static const ROOT_SCREEN:String = "root_screen";
      
      public static const CHAR_SCREEN:String = "char_screen";
      
      public static const NEW_CHAR_SCREEN:String = "new_char_screen";
      
      public static const BASE_SETTINGS:String = "base_settings";
      
      public static const SETTINGS_TUNE:String = "settings_tune";
      
      public static const SETTINGS_TUNE_VIDEO_MAIN:String = "settings_tune_video_main";
      
      public static const KEYBINDS_TUNE:String = "keybinds_tune";
      
      public static var TARGET_SCREEN:String = "";
      
      TARGET_SCREEN = "";
      
      public function MainMenuGUI()
      {
         super();
      }
      
      override protected function initGUI() : void
      {
         Logger.LogToChannel(Logger.DEBUG,"Base.initGUI");
         navigator.addScreen(new LoginScreen(LOGIN_SCREEN,100));
         navigator.addScreen(new FirstCharNameScreen(CHARNAME_SCREEN,402));
         navigator.addScreen(new MainScreen(ROOT_SCREEN,200));
         navigator.addScreen(new CharScreen(CHAR_SCREEN,300));
         navigator.addScreen(new NewCharScreen(NEW_CHAR_SCREEN,400));
         navigator.addScreen(new NewSettingsScreen(BASE_SETTINGS,301));
         navigator.addScreen(new SettingsTuneScreen(SETTINGS_TUNE,401));
         navigator.addScreen(new SettingsTuneVideoMainScreen(SETTINGS_TUNE_VIDEO_MAIN,410));
         navigator.addScreen(new SettingsKeyboardScreen(KEYBINDS_TUNE,450));
         navigator.showScreen(LOGIN_SCREEN);
         Auth.self.addEventListener(Auth.AUTH_SUCCESS,this.onAuthSuccess);
         Auth.self.addEventListener(Auth.AUTH_FAIL,this.onAuthFail);
         Auth.self.addEventListener(ScreenEvent.GO_SCREEN,this.goScreenHandler);
         navigator.addEventListener(ScreenEvent.GO_SCREEN,this.goScreenHandler,true);
         Api.self.addEventListener(Api.RESTART_GAME,this.onRestartGameHandler);
         Api.self.addEventListener(Api.ALERT,this.onAlertHandler);
         Api.self.addEventListener(Api.SHOW_NEWS,this.onNewsHandler);
         Api.self.addEventListener(Api.CONFIRM_WINDOW_SHOW,this.onConfirmWindowShowHandler);
         Api.self.addEventListener(Api.CONFIRM_WINDOW_HIDE,this.onConfirmWindowHideHandler);
         Api.self.addEventListener(Api.OPEN_SETTINGS,this.onOpenSettings);
         tabEnabled = false;
         tabChildren = false;
      }
      
      private function onOpenSettings(arg1:Object) : void
      {
         navigator.showScreen(MainMenuGUI.BASE_SETTINGS,true);
      }
      
      protected function goScreenHandler(arg1:ScreenEvent) : void
      {
         Logger.LogToChannel(Logger.DEBUG,this,"goScreenHandler " + arg1.data.toString());
         navigator.showScreen(arg1.data);
      }
      
      protected function onConfirmWindowShowHandler(arg1:ApiEvent) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"MainMenuGUI.onConfirmWindoShowHandler");
         navigator.showConfirmWindow(arg1);
      }
      
      protected function onConfirmWindowHideHandler(arg1:ApiEvent) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"MainMenuGUI.onConfirmWindoHideHandler");
         navigator.hideConfirmWindow();
      }
      
      protected function onNewsHandler(arg1:ApiEvent) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"MainMenuGUI.onNewsHandler");
         News.content = "<font color=\'#ffffff\' size=\'22\' face=\'GUILight\'>" + arg1.data.answer.text + "</font>";
         News.title = arg1.data.answer.title;
         News.source = arg1.data.answer.source;
         News.doNotShowNewsWindowAnymore = arg1.data.answer.showMeNewsEverytime == 0;
         Logger.LogToChannel(Logger.DEBUG,"MainMenuGUI.onNewsHandler2");
         var loc1:* = "<font color=\'#ffffff\' size=\'22\' face=\'GUIRegular\'>" + "<p>" + "Hello, this is <font color=\'#54bdff\'><a href=\'event:http://www.ya.ru\'>link</a>.</font>" + "</font>" + "</p>";
         Navigator.NEWS_DIALOG = Base.navigator.showNewsDialog(News.title,News.content,true,[new DialogButtonItem("extendedGUI.NewsWindow.openNewsArchive",this.openNewsArchive,0.3),new DialogButtonItem("extendedGUI.NewsWindow.closeDialog",this.doCloseNewsDialog,0.7,[Keyboard.ENTER,Keyboard.NUMPAD_ENTER])],Base.stage.stageWidth * 0.7,Base.stage.stageHeight * 0.7,true);
      }
      
      protected function openNewsArchive() : void
      {
         Logger.LogToChannel(Logger.DEBUG,"MainMenuGUI.openNewsArchive",News.source);
         Api.call(Api.OPEN_URL,[{"url":News.source}]);
         Navigator.NEWS_DIALOG.doClose();
      }
      
      protected function doCloseNewsDialog() : void
      {
         Navigator.NEWS_DIALOG.doClose();
      }
      
      protected function onAlertHandler(arg1:ApiEvent) : void
      {
         Base.navigator.showDialog(arg1.data.answer.message.title,arg1.data.answer.message.text,false,[new DialogButtonItem("extendedGUI.Dialogs.Ok",null,1,[Keyboard.ENTER])]);
      }
      
      protected function onRestartGameHandler(arg1:ApiEvent) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"onRestartGameHandler");
         Api.call(Api.DO_RESTART_GAME,[]);
      }
      
      protected function onRestartOkButton() : void
      {
         Logger.LogToChannel(Logger.DEBUG,this,"onRestartOkButton");
         Api.call(Api.DO_RESTART_GAME,[]);
      }
      
      protected function onAuthFail(arg1:Event) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"Auth.onAuthFail.");
         Base.setLoginBackgroundVisible(true);
         navigator.showScreen(LOGIN_SCREEN);
      }
      
      protected function onAuthSuccess(arg1:Event) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"Auth.onAuthSuccess.");
         Base.setLoginBackgroundVisible(false);
         navigator.showScreen(ROOT_SCREEN);
      }
   }
}

