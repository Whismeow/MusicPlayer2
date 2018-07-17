// FormatConvertDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "MusicPlayer2.h"
#include "FormatConvertDlg.h"
#include "afxdialogex.h"

CBASSEncodeLibrary CFormatConvertDlg::m_bass_encode_lib;

// CFormatConvertDlg 对话框

IMPLEMENT_DYNAMIC(CFormatConvertDlg, CDialog)

CFormatConvertDlg::CFormatConvertDlg(const vector<int>& items_selected, CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_FORMAT_CONVERT_DIALOG, pParent)
{
	//获取文件列表
	for (const auto& index : items_selected)
	{
		const SongInfo& song{ theApp.m_player.GetPlayList()[index] };
		m_file_list.push_back(theApp.m_player.GetCurrentDir() + song.file_name);
	}
}

CFormatConvertDlg::~CFormatConvertDlg()
{
	m_bass_encode_lib.UnInit();
}

void CFormatConvertDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SONG_LIST1, m_file_list_ctrl);
	DDX_Control(pDX, IDC_OUT_FORMAT_COMBO, m_encode_format_combo);
}


BEGIN_MESSAGE_MAP(CFormatConvertDlg, CDialog)
	ON_CBN_SELCHANGE(IDC_OUT_FORMAT_COMBO, &CFormatConvertDlg::OnCbnSelchangeOutFormatCombo)
	ON_BN_CLICKED(IDC_START_CONVERT_BUTTON, &CFormatConvertDlg::OnBnClickedStartConvertButton)
	ON_BN_CLICKED(IDC_BROWSE_BUTTON, &CFormatConvertDlg::OnBnClickedBrowseButton)
	ON_MESSAGE(WM_CONVERT_PROGRESS, &CFormatConvertDlg::OnConvertProgress)
	ON_MESSAGE(WM_CONVERT_COMPLETE, &CFormatConvertDlg::OnConvertComplete)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CFormatConvertDlg 消息处理程序


BOOL CFormatConvertDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化

	//初始化文件列表
	CRect rect;
	m_file_list_ctrl.GetClientRect(rect);
	int width0, width1, width2;
	width0 = rect.Width() / 10;
	width2 = rect.Width() * 2 / 10;
	width1 = rect.Width() - width0 - width2 - DPI(21);
	//插入列
	m_file_list_ctrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
	m_file_list_ctrl.InsertColumn(0, _T("序号"), LVCFMT_LEFT, width0);		//插入第1列
	m_file_list_ctrl.InsertColumn(1, _T("文件名"), LVCFMT_LEFT, width1);		//插入第2列
	m_file_list_ctrl.InsertColumn(2, _T("转换状态"), LVCFMT_LEFT, width2);		//插入第3列
	//插入项目
	for (size_t i{}; i<m_file_list.size(); i++)
	{
		CString tmp;
		tmp.Format(_T("%d"), i + 1);
		m_file_list_ctrl.InsertItem(i, tmp);
		CFilePathHelper file_path(m_file_list[i]);
		m_file_list_ctrl.SetItemText(i, 1, file_path.GetFileName().c_str());
	}
	//设置主题颜色
	m_file_list_ctrl.SetColor(theApp.m_app_setting_data.theme_color);

	//初始化转换格式的下拉列表
	m_encode_format_combo.AddString(_T("WAV"));
	m_encode_format_combo.AddString(_T("MP3 (lame编码器)"));
	m_encode_format_combo.AddString(_T("WMA"));
	m_encode_format_combo.AddString(_T("OGG"));
	m_encode_format_combo.SetCurSel(static_cast<int>(m_encode_format));


	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CFormatConvertDlg::EnableControls(bool enable)
{
	GetDlgItem(IDC_OUT_FORMAT_COMBO)->EnableWindow(enable);
	GetDlgItem(IDC_BROWSE_BUTTON)->EnableWindow(enable);
	GetDlgItem(IDC_START_CONVERT_BUTTON)->EnableWindow(enable);
}

bool CFormatConvertDlg::InitEncoder()
{
	//初始化bass encode库
	wstring dll_path;
#ifdef _DEBUG
	m_encode_dir = L".\\Encoder\\";
#else
	m_encode_dir = theApp.m_module_dir + L"Encoder\\";
#endif // _DEBUG
	dll_path = m_encode_dir + L"bassenc.dll";
	m_bass_encode_lib.Init(dll_path);
	bool successed = m_bass_encode_lib.IsSuccessed();
	if (!successed)
	{
		CString info;
		info.Format(_T("BASS 编码器加载失败，文件 %s 可能已经损坏！"), dll_path.c_str());
		MessageBox(info, NULL, MB_ICONERROR | MB_OK);
	}
	return successed;
}

void CFormatConvertDlg::EncodeSingleFile(const wstring & file_path, const wstring& out_dir, const wstring& encoder_dir, EncodeFormat format, int file_index, HWND hWnd)
{
	//创建解码流
	HSTREAM hStream = BASS_StreamCreateFile(FALSE, file_path.c_str(), 0, 0, BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT);
	if (hStream == 0)
	{
		::PostMessage(hWnd, WM_CONVERT_PROGRESS, file_index, -1);
		return;
	}
	bool is_midi = (CAudioCommon::GetAudioType(file_path) == AU_MIDI);
	if (theApp.m_player.m_bass_midi_lib.IsSuccessed() && is_midi && theApp.m_player.GetSoundFont().font != 0)
		theApp.m_player.m_bass_midi_lib.BASS_MIDI_StreamSetFonts(hStream, &theApp.m_player.GetSoundFont(), 1);
	//设置输出文件路径
	CFilePathHelper c_file_path(file_path);
	switch (format)
	{
	case EncodeFormat::WAV:
		c_file_path.ReplaceFileExtension(L"wav");
		break;
	case EncodeFormat::MP3:
		c_file_path.ReplaceFileExtension(L"mp3");
		break;
	case EncodeFormat::WMA:
		c_file_path.ReplaceFileExtension(L"wma");
		break;
	case EncodeFormat::OGG:
		c_file_path.ReplaceFileExtension(L"ogg");
		break;
	}
	wstring out_file_path;
	out_file_path = out_dir + c_file_path.GetFileName();
	//开始解码
	HENCODE hEncode;
	wstring cmdline;
	switch (format)
	{
	case EncodeFormat::MP3:
		cmdline = encoder_dir;
		cmdline += L"lame -h --preset standard - \"";
		cmdline += out_file_path;
		cmdline.push_back(L'\"');
		break;
	case EncodeFormat::OGG:
		cmdline = encoder_dir;
		cmdline += L"oggenc -q 3 -o \"";
		cmdline += out_file_path;
		cmdline += L"\" -";
		break;
	default:
		cmdline = out_file_path.c_str();
	}

	hEncode = m_bass_encode_lib.BASS_Encode_StartW(hStream, cmdline.c_str(), BASS_ENCODE_AUTOFREE | (format == EncodeFormat::WAV ? BASS_ENCODE_PCM : 0), NULL, 0);
	//获取流的长度
	QWORD length = BASS_ChannelGetLength(hStream, 0);
	char temp[20000];
	while (BASS_ChannelIsActive(hStream) != 0)
	{
		if (theApp.m_format_convert_dialog_exit)
		{
			m_bass_encode_lib.BASS_Encode_Stop(hEncode);
			BASS_StreamFree(hStream);
			return;
		}

		BASS_ChannelGetData(hStream, temp, 20000);
		if (m_bass_encode_lib.BASS_Encode_IsActive(hStream) == 0)
		{
			m_bass_encode_lib.BASS_Encode_Stop(hEncode);
			BASS_StreamFree(hStream);
		}

		//获取转换百分比
		QWORD position = BASS_ChannelGetPosition(hStream, 0);
		static int last_percent{ -1 };
		int percent = static_cast<int>(position * 100 / length);
		if (last_percent != percent)
			::PostMessage(hWnd, WM_CONVERT_PROGRESS, file_index, percent);
		last_percent = percent;
	}
	BASS_StreamFree(hStream);

	//switch (format)
	//{
	//case CFormatConvertDlg::EncodeFormat::WAV:
	//	break;
	//case CFormatConvertDlg::EncodeFormat::MP3:
	//	break;
	//case CFormatConvertDlg::EncodeFormat::WMA:
	//	break;
	//default:
	//	break;
	//}
}


UINT CFormatConvertDlg::ThreadFunc(LPVOID lpParam)
{
	CFormatConvertDlg* pThis{ (CFormatConvertDlg*)lpParam };
	for (size_t i{}; i < pThis->m_file_list.size(); i++)
	{
		if (theApp.m_format_convert_dialog_exit)
			return 0;
		EncodeSingleFile(pThis->m_file_list[i], pThis->m_out_dir, pThis->m_encode_dir, pThis->m_encode_format, i, pThis->GetSafeHwnd());
	}
	::PostMessage(pThis->GetSafeHwnd(), WM_CONVERT_COMPLETE, 0, 0);
	return 0;
}


void CFormatConvertDlg::OnCbnSelchangeOutFormatCombo()
{
	// TODO: 在此添加控件通知处理程序代码
	m_encode_format = static_cast<EncodeFormat>(m_encode_format_combo.GetCurSel());
}


void CFormatConvertDlg::OnBnClickedStartConvertButton()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!InitEncoder())
		return;
	if (m_out_dir.empty())
	{
		MessageBox(_T("请设置输出目录！"), NULL, MB_ICONWARNING | MB_OK);
		return;
	}
	else if (!CCommon::FolderExist(m_out_dir))
	{
		CString info;
		info.Format(_T("输出目录 \"%s\" 不存在！"), m_out_dir.c_str());
		MessageBox(info, NULL, MB_ICONWARNING | MB_OK);
	}

	//先清除“状态”一列的内容
	for (size_t i{}; i < m_file_list.size(); i++)
	{
		m_file_list_ctrl.SetItemText(i, 2, _T(""));
	}

	EnableControls(false);
	theApp.m_format_convert_dialog_exit = false;
	//创建格式转换的工作线程
	m_pThread = AfxBeginThread(ThreadFunc, this);

}


void CFormatConvertDlg::OnBnClickedBrowseButton()
{
	// TODO: 在此添加控件通知处理程序代码
#ifdef COMPILE_IN_WIN_XP
	CFolderBrowserDlg folderPickerDlg(this->GetSafeHwnd());
	folderPickerDlg.SetInfo(_T("请选择输出文件夹。"));
#else
	CFolderPickerDialog folderPickerDlg(m_out_dir.c_str());
	folderPickerDlg.m_ofn.lpstrTitle = _T("选择输出文件夹");		//设置对话框标题
#endif // COMPILE_IN_WIN_XP
	if (folderPickerDlg.DoModal() == IDOK)
	{
		m_out_dir = folderPickerDlg.GetPathName();
		if (m_out_dir.back() != L'\\') m_out_dir.push_back(L'\\');	//确保路径末尾有反斜杠
		SetDlgItemText(IDC_OUT_DIR_EDIT, m_out_dir.c_str());
	}

}


afx_msg LRESULT CFormatConvertDlg::OnConvertProgress(WPARAM wParam, LPARAM lParam)
{
	CString percent_str;
	int percent = (int)lParam;
	if(percent == -1)
		percent_str = _T("转换出错");
	else
		percent_str.Format(_T("%d%%"), (int)lParam);
	m_file_list_ctrl.SetItemText(wParam, 2, percent_str);

	return 0;
}


afx_msg LRESULT CFormatConvertDlg::OnConvertComplete(WPARAM wParam, LPARAM lParam)
{
	EnableControls(true);
	return 0;
}


void CFormatConvertDlg::OnCancel()
{
	// TODO: 在此添加专用代码和/或调用基类
	//theApp.m_format_convert_dialog_exit = true;
	//if (m_pThread != nullptr)
	//	WaitForSingleObject(m_pThread->m_hThread, 2000);	//等待线程退出

	CDialog::OnCancel();
}


void CFormatConvertDlg::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类
	//theApp.m_format_convert_dialog_exit = true;
	//if (m_pThread != nullptr)
	//	WaitForSingleObject(m_pThread->m_hThread, 2000);	//等待线程退出

	CDialog::OnOK();
}


void CFormatConvertDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	theApp.m_format_convert_dialog_exit = true;
	if (m_pThread != nullptr)
		WaitForSingleObject(m_pThread->m_hThread, 2000);	//等待线程退出

	CDialog::OnClose();
}
