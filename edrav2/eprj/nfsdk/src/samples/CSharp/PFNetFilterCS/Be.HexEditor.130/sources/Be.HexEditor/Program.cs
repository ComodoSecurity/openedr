using System;
using System.Windows.Forms;
using System.Threading;
using System.Globalization;
using Be.HexEditor.Properties;

namespace Be.HexEditor
{
    class Program
    {
        public const string SOFTWARENAME = "Be.HexEditor";

        public static FormHexEditor FormHexEditor;

        [STAThread()]
        static void Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            if (!Settings.Default.UseSystemLanguage)
            {
                string l = Settings.Default.SelectedLanguage;
                Thread.CurrentThread.CurrentUICulture = new CultureInfo(l);
                Thread.CurrentThread.CurrentCulture = CultureInfo.CreateSpecificCulture(l);
            }

            FormHexEditor = new FormHexEditor();
            if (args.Length > 0 && System.IO.File.Exists(args[0]))
                FormHexEditor.OpenFile(args[0]);
            Application.Run(FormHexEditor);
        }

        public static DialogResult ShowError(Exception ex)
        {
            return ShowError(ex.Message);
        }


        internal static DialogResult ShowError(string text)
        {
            DialogResult result = MessageBox.Show(text, SOFTWARENAME,
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return result;
        }

        public static void ShowMessage(string text)
        {
            MessageBox.Show(text, SOFTWARENAME,
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }

        public static DialogResult ShowQuestion(string text)
        {
            DialogResult result = MessageBox.Show(text, SOFTWARENAME,
               MessageBoxButtons.YesNo,
               MessageBoxIcon.Question);
            return result;
        }
    }
}
