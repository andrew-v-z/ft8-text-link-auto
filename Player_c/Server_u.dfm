object Form1: TForm1
  Left = 0
  Top = 0
  Caption = #1057#1077#1088#1074#1077#1088' '#1079#1074#1091#1082#1072
  ClientHeight = 248
  ClientWidth = 556
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  TextHeight = 15
  object Label1: TLabel
    Left = 455
    Top = 152
    Width = 35
    Height = 15
    Caption = #1042#1088#1077#1084#1103
  end
  object Label2: TLabel
    Left = 455
    Top = 91
    Width = 29
    Height = 15
    Caption = #1058#1077#1082#1089#1090
  end
  object Label3: TLabel
    Left = 455
    Top = 27
    Width = 65
    Height = 15
    Caption = #1050#1086#1083#1080#1095#1077#1089#1090#1074#1086
  end
  object txtResults: TMemo
    Left = 8
    Top = 8
    Width = 441
    Height = 233
    TabOrder = 0
  end
  object txtCount: TEdit
    Left = 455
    Top = 48
    Width = 89
    Height = 23
    TabOrder = 1
  end
  object txtFrom: TEdit
    Left = 455
    Top = 112
    Width = 89
    Height = 23
    TabOrder = 2
  end
  object PerformEd: TEdit
    Left = 455
    Top = 176
    Width = 89
    Height = 23
    TabOrder = 3
  end
  object IdTCPServer1: TIdTCPServer
    Active = True
    Bindings = <
      item
        IP = '0.0.0.0'
        Port = 31777
      end>
    DefaultPort = 0
    OnExecute = IdTCPServer1Execute
    Left = 40
    Top = 88
  end
end
