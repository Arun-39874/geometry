#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "libs/geofun.h"
#include "libs/matfun.h"

#define BUFFER_SIZE 2048

void sendHTMLForm(int client_fd, int x1, int y1, int x2, int y2, int x3, int y3, double sideAB, double sideBC, double sideCA, double angleA, double angleB, double angleC) {
    char *html_template = "<!DOCTYPE html>\n"
                          "<html lang=\"en\">\n"
                          "<head>\n"
                          "<meta charset=\"UTF-8\">\n"
                          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                          "<title>Triangle Properties</title>\n"
                          "</head>\n"
                          "<body>\n"
                          "<style>"
    "html {font-family: Times New Roman; display: inline-block; text-align: center;}"
    "h2 {font-size: 2.0rem; color: blue;}"
  "</style>" 
                          "<h2>Side and Angle</h2>\n"
                          "<form method=\"post\">\n"
                          "    <label for=\"x1\">Vertex A (x1 y1):</label><br>\n"
                          "    <input type=\"text\" id=\"x1\" name=\"x1\" value=\"%d\" required>\n"
                          "    <input type=\"text\" id=\"y1\" name=\"y1\" value=\"%d\" required><br><br>\n"
                          "    <label for=\"x2\">Vertex B (x2 y2):</label><br>\n"
                          "    <input type=\"text\" id=\"x2\" name=\"x2\" value=\"%d\" required>\n"
                          "    <input type=\"text\" id=\"y2\" name=\"y2\" value=\"%d\" required><br><br>\n"
                          "    <label for=\"x3\">Vertex C (x3 y3):</label><br>\n"
                          "    <input type=\"text\" id=\"x3\" name=\"x3\" value=\"%d\" required>\n"
                          "    <input type=\"text\" id=\"y3\" name=\"y3\" value=\"%d\" required><br><br>\n"
                          "    <button type=\"submit\">Calculate</button>\n"
                          "</form>\n"
                          "<h3>Results</h3>\n"
                          "<p>Side AB: %.2f</p>\n"
                          "<p>Side BC: %.2f</p>\n"
                          "<p>Side CA: %.2f</p>\n"
                          "<p>Angle A: %.2f degrees</p>\n"
                          "<p>Angle B: %.2f degrees</p>\n"
                          "<p>Angle C: %.2f degrees</p>\n"
                          "</body>\n"
                          "</html>\n";

    char response[BUFFER_SIZE];
    sprintf(response, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
    snprintf(response + strlen(response), BUFFER_SIZE - strlen(response), html_template, x1, y1, x2, y2, x3, y3, sideAB, sideBC, sideCA, angleA, angleB, angleC);
    send(client_fd, response, strlen(response), 0);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port 8080\n");

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("New connection accepted\n");

        char buffer[BUFFER_SIZE] = {0};
        read(client_fd, buffer, BUFFER_SIZE);
        printf("Received data from client: %s\n", buffer);

        char *body_start = strstr(buffer, "\r\n\r\n");

        if (body_start) {
            body_start += 4;

            int x1, y1, x2, y2, x3, y3, m=2, n=1;
            sscanf(body_start, "x1=%d&y1=%d&x2=%d&y2=%d&x3=%d&y3=%d", &x1, &y1, &x2, &y2, &x3, &y3);
            double **A,**B,**C,sideAB, sideBC, sideCA;
	    A = createMat(m,n);
            B = createMat(m,n);
            C = createMat(m,n);
            A[0][0] = x1;
            A[1][0] = y1;
            B[0][0] = x2;
            B[1][0] = y2;
            C[0][0] = x3;
            C[1][0] = y3;
            double **s_ab, **s_bc, **s_ca;
	    s_ab = Matsub(A,B,m,n);//A-B
            s_bc = Matsub(B,C,m,n);//B-C
            s_ca = Matsub(C,A,m,n);//C-A
            sideAB = Matnorm(s_ab,m);
            sideBC = Matnorm(s_bc,m); 
            sideCA = Matnorm(s_ca,m);
    

            double angleA = calculateAngle(sideBC, sideCA, sideAB) * 180 / M_PI;
            double angleB = calculateAngle(sideCA, sideAB, sideBC) * 180 / M_PI;
            double angleC = calculateAngle(sideAB, sideBC, sideCA) * 180 / M_PI;

            sendHTMLForm(client_fd, x1, y1, x2, y2, x3, y3, sideAB, sideBC, sideCA, angleA, angleB, angleC);
            printf("Results sent to the client\n");
        }

        close(client_fd);
    }

    return 0;
}
/*double calculateAngle(double a, double b, double c) {
    return acos((b * b + c * c - a * a) / (2 * b * c));
}*/
