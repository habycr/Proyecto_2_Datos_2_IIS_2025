using System;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using UI.Models;
using UI.Services;

namespace UI
{
    public partial class MainWindow : Window
    {
        private ProblemApiClient _apiClient;
        private ProblemDto? _currentProblem;

        public MainWindow()
        {
            InitializeComponent();

            // Inicializar el cliente API
            _apiClient = new ProblemApiClient("http://localhost:8080");

            // Cargar la lista de problemas al iniciar
            Loaded += MainWindow_Loaded;
        }

        // ==================== Evento: Ventana cargada ====================
        private async void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            await LoadProblemListAsync();
        }

        // ==================== Cargar lista de problemas ====================
        private async System.Threading.Tasks.Task LoadProblemListAsync()
        {
            try
            {
                ConsoleOutput.Text = "🔄 Conectando al Gestor de Problemas...\n";

                // Verificar salud del servidor
                bool isHealthy = await _apiClient.IsHealthyAsync();
                if (!isHealthy)
                {
                    ConsoleOutput.Text += "❌ ERROR: No se puede conectar al servidor en http://localhost:8080\n";
                    ConsoleOutput.Text += "   → Asegúrate de que 'problem_manager_api.exe' esté corriendo.\n";
                    ConsoleOutput.Text += "   → Verifica que MongoDB esté activo.\n";
                    return;
                }

                ConsoleOutput.Text += "✅ Conexión exitosa al servidor.\n";

                // Obtener todos los problemas
                var problems = await _apiClient.GetAllProblemsAsync();

                if (problems.Count == 0)
                {
                    ConsoleOutput.Text += "\n📭 No hay problemas en la base de datos.\n";
                    ConsoleOutput.Text += "   → Puedes crear algunos usando el endpoint POST /problems\n";
                    ProblemList.ItemsSource = null;
                    return;
                }

                // Asignar al ListBox (mostrando solo los títulos)
                ProblemList.ItemsSource = problems;
                ProblemList.DisplayMemberPath = "title";

                ConsoleOutput.Text += $"✅ {problems.Count} problema(s) cargado(s) correctamente.\n\n";
                ConsoleOutput.Text += "💡 Selecciona un problema de la lista para ver sus detalles.\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $"❌ ERROR: {ex.Message}\n";
                ConsoleOutput.Text += $"   Detalles: {ex.StackTrace}\n";
            }
        }

        // ==================== Evento: Problema seleccionado ====================
        private async void ProblemList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ProblemList.SelectedItem is not ProblemDto selectedProblem)
                return;

            try
            {
                ConsoleOutput.Text = $"📥 Cargando detalles de '{selectedProblem.title}'...\n";

                // Obtener detalle completo del problema
                var fullProblem = await _apiClient.GetProblemAsync(selectedProblem.problem_id);

                if (fullProblem == null)
                {
                    ConsoleOutput.Text += "❌ ERROR: No se pudo cargar el problema completo.\n";
                    return;
                }

                _currentProblem = fullProblem;

                // Mostrar descripción en el panel de descripción
                DescriptionText.Text = $"📝 {fullProblem.title}\n\n" +
                                      $"🏷️  ID: {fullProblem.problem_id}\n" +
                                      $"📊 Dificultad: {fullProblem.difficulty}\n" +
                                      $"🔖 Tags: {string.Join(", ", fullProblem.tags)}\n\n" +
                                      $"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" +
                                      $"{fullProblem.description}\n\n" +
                                      $"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" +
                                      $"📋 Ejemplos de Prueba:\n\n";

                // Agregar casos de prueba a la descripción
                for (int i = 0; i < fullProblem.test_cases.Count; i++)
                {
                    var tc = fullProblem.test_cases[i];
                    DescriptionText.Text += $"   Ejemplo {i + 1}:\n";
                    DescriptionText.Text += $"   Input:  {tc.input}\n";
                    DescriptionText.Text += $"   Output: {tc.expected_output}\n\n";
                }

                // Cargar el code_stub en el editor
                if (!string.IsNullOrWhiteSpace(fullProblem.code_stub))
                {
                    CodeEditor.Text = fullProblem.code_stub;
                }
                else
                {
                    CodeEditor.Text = "// Escribe tu solución aquí\n";
                }

                // Actualizar consola
                ConsoleOutput.Text += $"✅ Problema cargado exitosamente.\n";
                ConsoleOutput.Text += $"📝 {fullProblem.test_cases.Count} caso(s) de prueba disponible(s).\n";
                ConsoleOutput.Text += $"\n💡 Escribe tu código en el editor y presiona 'Run' para probar.\n";
            }
            catch (Exception ex)
            {
                ConsoleOutput.Text += $"❌ ERROR: {ex.Message}\n";
            }
        }

        // ==================== Botón: Run ====================
        private void Run_Click(object sender, RoutedEventArgs e)
        {
            if (_currentProblem == null)
            {
                ConsoleOutput.Text = "❌ ERROR: Primero selecciona un problema de la lista.\n";
                return;
            }

            string userCode = CodeEditor.Text;

            if (string.IsNullOrWhiteSpace(userCode))
            {
                ConsoleOutput.Text = "❌ ERROR: El editor de código está vacío.\n";
                return;
            }

            ConsoleOutput.Text = $"▶️  Ejecutando código para '{_currentProblem.title}'...\n\n";
            ConsoleOutput.Text += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            ConsoleOutput.Text += "📝 Código a ejecutar:\n\n";
            ConsoleOutput.Text += userCode + "\n\n";
            ConsoleOutput.Text += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
            ConsoleOutput.Text += "⏳ [PENDIENTE] Conectar con el Motor de Evaluación\n";
            ConsoleOutput.Text += "   → Endpoint: POST http://localhost:8081/run\n";
            ConsoleOutput.Text += "   → Esta funcionalidad se implementará en la siguiente fase.\n";

            // TODO: Llamar al Motor de Evaluación cuando esté listo
            // var motorClient = new MotorApiClient("http://localhost:8081");
            // var result = await motorClient.RunCodeAsync(_currentProblem.problem_id, userCode);
        }

        // ==================== Botón: Submit ====================
        private void Submit_Click(object sender, RoutedEventArgs e)
        {
            if (_currentProblem == null)
            {
                ConsoleOutput.Text = "❌ ERROR: Primero selecciona un problema de la lista.\n";
                return;
            }

            string userCode = CodeEditor.Text;

            if (string.IsNullOrWhiteSpace(userCode))
            {
                ConsoleOutput.Text = "❌ ERROR: El editor de código está vacío.\n";
                return;
            }

            ConsoleOutput.Text = $"✅ Enviando solución para '{_currentProblem.title}'...\n\n";
            ConsoleOutput.Text += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            ConsoleOutput.Text += "📝 Código enviado:\n\n";
            ConsoleOutput.Text += userCode + "\n\n";
            ConsoleOutput.Text += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
            ConsoleOutput.Text += "⏳ [PENDIENTE] Conectar con el Analizador de Soluciones\n";
            ConsoleOutput.Text += "   → Endpoint: POST http://localhost:8082/submit\n";
            ConsoleOutput.Text += "   → Esta funcionalidad se implementará en la siguiente fase.\n";

            // TODO: Llamar al Analizador cuando esté listo
            // var analizadorClient = new AnalizadorApiClient("http://localhost:8082");
            // var feedback = await analizadorClient.AnalyzeCodeAsync(_currentProblem.problem_id, userCode);
        }

        // ==================== Botón: Abrir Modo Administrador ====================
        private void AdminButton_Click(object sender, RoutedEventArgs e)
        {
            var adminWindow = new AdminWindow();
            adminWindow.ShowDialog(); // Modal: bloquea MainWindow hasta que se cierre

            // Opcional: recargar la lista después de cerrar Admin
            // _ = LoadProblemListAsync();
        }

    }
}