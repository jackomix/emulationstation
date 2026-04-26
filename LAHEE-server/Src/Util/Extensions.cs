using System.Web;
using Newtonsoft.Json;
using WatsonWebserver.Core;

namespace LAHEE.Util;

static class Extensions {
    public static string GetParameter(this HttpRequestBase req, string str) {
        if (req.Query != null && req.Query.Elements != null) {
            string val = req.Query.Elements[str];
            if (val != null) return val;
        }

        if (req.DataAsString == null) {
            return null;
        }

        return HttpUtility.ParseQueryString(req.DataAsString)[str];
    }

    public static async Task SendJson(this HttpResponseBase resp, object obj) {
        string data = JsonConvert.SerializeObject(obj);
        await resp.Send(data);
    }

    public static IEnumerable<Exception> GetInnerExceptions(this Exception ex) {
        if (ex == null) {
            throw new ArgumentNullException(nameof(ex));
        }

        Exception innerException = ex;
        do {
            yield return innerException;
            innerException = innerException.InnerException;
        } while (innerException != null);
    }
}