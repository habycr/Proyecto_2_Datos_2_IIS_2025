using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace UI
{
    public partial class MainWindow : Window
    {
        private ApiClient api;

        public MainWindow()
        {
            InitializeComponent();

            api = new ApiClient
            {
                GestorProblemasURL = "http://localhost:8080",
                MotorEvaluacionURL = "http://localhost:8081",
                AnalizadorURL = "http://localhost:8082"
            };

            //LoadProblemList();
        }

        // Asegúrate de que el método LoadProblemList() esté implementado en esta clase.

        // Agrega el siguiente método para manejar el evento SelectionChanged
        private void ProblemList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ProblemList.SelectedItem is ListBoxItem item)
            {
                ConsoleOutput.Text += $"Seleccionaste: {item.Content}\n";
            }

            // Cuando conectes el API, aquí harás:
            // api.GetProblemDetail(id);
        }
        private void Run_Click(object sender, RoutedEventArgs e)
        {
            ConsoleOutput.Text += "Run presionado\n";
        }

        private void Submit_Click(object sender, RoutedEventArgs e)
        {
            ConsoleOutput.Text += "Submit presionado\n";
        }

    }
}